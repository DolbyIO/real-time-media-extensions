package main

import (
  "bytes"
  "context"
  "encoding/binary"
  "errors"
  "io"
  "log"
  "os"
  "os/exec"
  "os/signal"
  "path/filepath"
  "strconv"
  "syscall"
  "time"
  "sync/atomic"

  "github.com/DolbyIO/real-time-media-extensions/pkg/messenger"
  "github.com/DolbyIO/real-time-media-extensions/pkg/common"
  "github.com/labstack/echo/v4"
)

const (
  // Exit codes
  AppNormalExitCode           = 0
  AppShutdownExitCode         = 6
  AppFailureTerminalExitCode  = 7
  AppFailureRestartExitCode   = 8

  // Http endpoints
  PathHealth                  = "/health"
  PathReady                   = "/ready"
  PathTerminate               = "/terminate"
  PathStart                   = "/start"
  PathStop                    = "/stop"

  // Binary locations
  RelativePathPlugin          = "lib/libdolbyio_comms_transcription.so"
  RelativePathDesktopApp      = "bin/desktop_app"
)

func GetEnvOrDefault(env string, def string) string {
  if v, ok := os.LookupEnv(env); ok {
    return v
  }
  return def
}

var (
  BinaryLocation = GetEnvOrDefault("BINARY_LOCATION", "/opt/rtme/")
  TranscriptDestination = GetEnvOrDefault("TRANSCRIPT_DESTINATION", "")
)

// Thread safe globals used for stuffs. I hate that this is
// global but like when I learn go I'll fix it.
var gAppExitCode int64
var gShuttingDown = common.NewSafeInteger(0, 0)
var gAppProcessId = common.NewSafeInteger(-1, -1)

// Shuts down the application with SIGTERM, at least tries too. If this
// timesout we send a SIGKILL.
func ShutdownProcess(process *os.Process) {
  log.Println("Calling shutdown off application process!")
  if gShuttingDown.Boolean() {
    log.Println("Already in the process of shutting down!")
    return
  }
  gShuttingDown.Set(1)
  process.Signal(syscall.SIGTERM)
  go func() {
    time.Sleep(10 * time.Second)
    process.Signal(syscall.SIGKILL)
  }()
  state, _ := process.Wait()
  if ProcessExitCode(int64(state.ExitCode())) {
    log.Println("Exited cleanly application!")
  } else {
    log.Println("Improper exit of application!");
  }
}

// I think we can do without this function. This was left over
// from when we wanted essentially a single shot run of things.
func ProcessExitCode(exit_code int64) bool {
  atomic.AddInt64(&gAppExitCode, exit_code)
  if exit_code <= AppShutdownExitCode {
    return true
  } else {
    return false
  }
}

// Checks to see if the application process exists. If it does
// make sure no one else can find existing version of this running
// process. Return pointer to process struct. 
func ExclusiveFindAppProcess() (*os.Process, error) {
  proc_id := gAppProcessId.GetReset()
  log.Println("Value of the process id: ", proc_id)
  if proc_id == -1 {
    return nil, errors.New("Process does not exist")
  }
  // On Linux this thing will always return some process and nil
  process, _ := os.FindProcess(proc_id)
  if err := process.Signal(syscall.Signal(0)); err != nil {
    return nil, errors.New("Process already dead!")
  }
  return process, nil
}

// Not really sure what we should do here. Since the pod can now be alive
// and start/stop the underlying c++ app on demand I am not sure how we can
// report health. Like if there is no transcript for some period of time it
// could just be that no one is talking not necessarily that there is an issue.
func HealthHandler(c echo.Context) error {
  return c.String(200, "Application is healthy")
}
func ReadyHandler(c echo.Context) error {
  return c.String(200, "Application is ready and started")
}

// Start the transcription application. The service expects the URL in the following
// formats for example:
//       https:X.Y.Z.U:8080?alias=val&token=val&username=val&service=gladia&gladiakey=abcdefgh
//       https:X.Y.Z.U:8080?alias=val&token=val&username=val&service=aws
//       https:X.Y.Z.U:8080?id=val&token=val&username=val&service=aws 
// From here the transcription application is launched, the transcript fifo is opened
// and the function for handling transcription is executed.
func StartHandler(c echo.Context) error {
  log.Println("Got start request")
  if (gAppProcessId.Get() > -1) {
    log.Println("Currently only supported one app at atime")
    return c.String(404, "Transcription application already running!");
  }
  alias := c.QueryParam("alias"); id := c.QueryParam("id")
  token := c.QueryParam("token"); user := c.QueryParam("username")
  service := c.QueryParam("service"); sender := c.QueryParam("sendmechanism")
  if len(service) == 0 {
    service = "gladia"
  }
  if service != "gladia" && service != "aws" {
    log.Println("Unsupported transcription service requested")
    return c.String(404, "Unsupported transcription service requested")
  }
  api_key := "api-key:"
  if service == "gladia" {
    api_key += c.QueryParam("gladiakey")
    if api_key == "api-key:" {
      log.Println("App key must be provided when using Gladia.io")
      return c.String(404, "App key must be provided when using Gladia.io")
    }
  } else {
    api_key += "x"
  }

  conf_access := "-c"; conf_value := alias
  if len(id) != 0 {
    conf_access = "-i"
    conf_value = id
  }

  transcript_file := filepath.Join("/tmp/", "transcript-" + conf_value + "-" +
               time.Now().UTC().Format("2006-01-02_15:04:05"))
  transcript_transfer_method := "--rtme-transcription-fifo-path"
  var message_sender messenger.MessageSender
  log.Println(TranscriptDestination)
  if TranscriptDestination == "testing_fifo" || len(sender) != 0 {
    log.Println("Creating the trancsript transfer fifo")
    err := syscall.Mkfifo(transcript_file, 0666)
    if err != nil {
      return c.String(404, "Failed to create the Fifo for transcript transferring!")
    }
    if len(sender) != 0 {
      if sender != "http" && sender != "pubnub" {
        log.Println("Only support sending to REST endpoint or using Pubnub")
        return c.String(404, "Only support sending to REST endpoint or using Pubnub")
      }
      message_sender = messenger.Create(sender, "myuniqueid")
      if sender == "pubnub" {
        pubnub_publish_key := c.QueryParam("publishkey")
        pubnub_subscribe_key := c.QueryParam("subscribekey")
        message_sender.Configure("publish", pubnub_publish_key)
        message_sender.Configure("subscribe", pubnub_subscribe_key)
      }
      message_sender.Initialize()
    }
  } else {
    transcript_transfer_method = "--rtme-transcription-file-path"
    log.Println("No outside destination provided for transcripts, they will stored in:", transcript_file)
  }

  log.Println("Starting transcription app conf: ", conf_value, user, transcript_file)
  args := []string{"--plugin", filepath.Join(BinaryLocation, RelativePathPlugin),
                    conf_access,
                    conf_value,
                    "-u", user,
                    "-k", token,
                    "-m", "NONE",
                    "-l", "3",
                    "--noninteractive",
                    transcript_transfer_method, transcript_file,
                    "--rtme-transcription-logging-level", "3",
                    "--rtme-transcription-service", service,
                    "--rtme-transcription-param", api_key,
                    };
  process := exec.Command(BinaryLocation + RelativePathDesktopApp, args...)
  process.Stdout = os.Stdout
  process.Stderr = os.Stderr
  err := process.Start()
  if err != nil {
    return c.String(404, "Failed to start the process")
  }
  gAppProcessId.Set(process.Process.Pid)
  gShuttingDown.Set(0)
  atomic.AddInt64(&gAppExitCode, -1)
  if message_sender != nil || TranscriptDestination == "testing_fifo" {
    if len(TranscriptDestination) == 0 {
      return c.String(404, "Trying to send transcripts but no destination configured")
    }
    go StartTranscriptionHandler(transcript_file, message_sender)
  }
  return c.String(200, "process started ok PID: " + strconv.Itoa(process.Process.Pid))
}

// Termination handler which handles the terminate/stop endpoints.
// It will check if there is a c++ application running and if this is
// the case it will terminate it reporting the status and logging the
// exit code.
func TerminateHandler(c echo.Context) error {
  log.Println("Received termination http request. Performing cleanup...")
  process, err := ExclusiveFindAppProcess()
  if err != nil {
    return c.String(404, err.Error())
  }
  ShutdownProcess(process)
  return c.String(200, "Terminated successfully")
}

// The go section of handling the transcription output. The c++ applicaton
// will write each transcript to the fifo in the following format:
// NUM_BYTES{TRANSCRIPT_JSON_FORMAT}
// NUM_BYTES: 4 byte integer specifying the length of json string that following
// TRANSCRIPTION_JSON_FORMAT: transcript + meta data in json format
// Note nothing is escaped/base64 encoded so probably should be done so.
func StartTranscriptionHandler(fifo_file string, msg_sender messenger.MessageSender) {
  log.Println("Startng the transcription handler")
  transcript_fifo, err := os.OpenFile(fifo_file, os.O_RDONLY, os.ModeNamedPipe)
  defer transcript_fifo.Close()
  if err != nil {
    log.Println("Failed to open the transcript fifo file!")
    return;
  }
  var continuous_read_fails int
  for {
    if continuous_read_fails > 0 {
      if gShuttingDown.Boolean() {
        log.Println("Application has been stopped exiting")
        break
      } else if continuous_read_fails > 10 {
        log.Println("Error 10+ continuous read failures, leaving the loop")
        break
      }
    }
    buffer := make([]byte, 4)
    _, err := io.ReadFull(transcript_fifo, buffer)
    if err != nil {
      log.Println("Failed to read bytes from file", err)
      continuous_read_fails++
      continue
    }
    var to_read int32
    err = binary.Read(bytes.NewReader(buffer), binary.LittleEndian, &to_read)
    if err != nil || to_read == -1 {
      log.Println("Breaking from the reading transcript loop")
      break
    }
    data := make([]byte, to_read)
    _, err = io.ReadFull(transcript_fifo, data)
    if err != nil {
      log.Println("Failed to read string from file", err)
      continuous_read_fails++
      continue
    }
    log.Printf("Read the following data: %s", data)
    continuous_read_fails = 0
    if TranscriptDestination != "testing_fifo" {
      msg_sender.SendMessage(data, TranscriptDestination)
    }
  }
}

func main() {
  gAppProcessId.Set(-1)
  gAppExitCode = -1
  e := echo.New()

  // Setup all the routes handled by go service
  e.GET(PathTerminate, TerminateHandler)
  e.GET(PathHealth, HealthHandler)
  e.GET(PathReady, ReadyHandler)
  e.GET(PathStart, StartHandler)
  e.GET(PathStop, TerminateHandler)

  // Signals for exiting the go service
  quit := make(chan os.Signal, 1)
  signal.Notify(quit, os.Interrupt, syscall.SIGTERM)
  signal.Notify(quit, os.Interrupt, syscall.SIGINT)
  signal.Notify(quit, os.Interrupt, syscall.SIGQUIT)

  // Start the server
  go func() {
    if err := e.Start("0.0.0.0:8080"); err != nil {
			e.Logger.Info("Shutting down the server")
		}
  }()

  for {
    signal_result := <-quit
    if signal_result == syscall.SIGINT ||
       signal_result == syscall.SIGTERM ||
       signal_result == syscall.SIGQUIT {
      process, err := ExclusiveFindAppProcess()
      if process != nil && err == nil {
        ShutdownProcess(process)
      }
      log.Println("Closing due to signal to service:", signal_result)
      ctxt, cancel := context.WithTimeout(context.Background(), 10 * time.Second)
      defer cancel()
      if err := e.Shutdown(ctxt); err != nil {
		    e.Logger.Fatal(err)
	    }
      break;
    }
  }
  os.Exit(int(atomic.LoadInt64(&gAppExitCode)))
}
