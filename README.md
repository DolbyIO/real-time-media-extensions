# Dolby.io Realtime Media Extensions

The Dolby.io Realtime Media Extensions provide a complete solution for
automated, realtime media processing in the Dolby.io infrastructure. Each
component provides its own API, but the project as a whole provides also the
ready-to-use application, as well as the Docker image and Kubernetes pod
definition.

## Building

### Build system requirements and prerequisites

- Ubuntu 20.04
- CMake 3.21

After unpacking the source, you should install the latest C++ SDK release for
Ubuntu 20.04, with GNU ABI, in the ext-lib/sdk-release-ubuntu-20.04-gcc-gnustl
in the unpacked source directory. This is the only manual step, the build script
will handle installing other dependencies and building the project.

### Building the Ubuntu 20.04 application and all components

Run the `setup/build.sh` script. It will download and build other dependencies,
like the AWS SDK. The script will create three directories:

- `build_dir` (the CMake build directory)
- `install_dir` (the installation directory of the project)
- `build_docker_dir` (the pre-packaging directory for creating a docker
  image)

The contents of the `install_dir` contain the build artifacts, dependencies and
public headers of the RTME project, but it does not contain the C++ SDK
libraries or the desktop_app. The `build_docker_dir` contains everything.

In result of a successful build, the `transcription` docker image is created and
the tarball containing it is compressed in the `build_docker_dir`.

### Building directly from CMake

If the `setup/build.sh` script has been run at least once, all required
dependencies in the `ext-lib` directory are prepared. At this point, it's
possible to build the RTME project directly from CMake with no custom config
params required:

```
	cmake -GNinja <RTME_source_directory> -DCMAKE_BUILD_TYPE=RelWithDebInfo
	cmake --build .
```

The RTME project can be included as a subproject in another CMake project, and
the installed artifacts contain CMake module definitions for easily importing
prebuilt components into your CMake project.

## Running

The RTME application can be run directly on the Ubuntu host (build machine) or
as a docker image. Running from the docker container exposes some REST API
endpoints, while running directly allows for quick iteration times.

The RTME plugin for transcription builds on the AWS transcribe service, so it
requires AWS credentials. The way these should be passed differs for docker
run and for direct application run, and it is described below.

### Running directly from build / build_docker_dir

When running directly (not with `docker run`) you will be launching the
`desktop_app` from the C++ SDK package. The app requires a few command line
parameters:

- the token, provided using the `-k` switch
- the conference ID or alias (`-i` or `-c` switches)
- the user name (`-u` switch)
- the plugin path (`--plugin` switch)
- optionally, the join options - `-m none` switch will join just as listener,
  with no audio or video sending enabled.
- optionally, the plugin-specific command-line switches
    - the transcription service to use (`--rtme-transcription-service` switch)
    - the service specific parameters to use (`--rtme-transcription-param` switch)

The RTME project provides the `libdolbyio_comms_transcription.so` plugin. Locate
it in the build directory, and then run the `desktop_app` as in:

```
desktop_app -k <token> -u test_user -c <conference_alias> --plugin <path_to_dolbyio_transcription> --rtme-transcription-service <transcription_service> -m none --rtme-transcription-param param1:value1
```

The application will join the conference, create one if needed, and will start
submitting the conference audio to the Gladia.io or AWS transcribe service, and 
handling the returned transcriptions by sending conference messages.

To use the Gladia.io transcription service the plugin requires an API key in order
to connect to the Gladia.io service. This api key must be provided as follows 
`--rtme-transcription-parameter api-key:VALUE`.

To use AWS service proper credentials for AWS must be accessible by the plugin. Also make sure 
the AWS role being authenticated has access to the AWS Transcribe service. The plugin itself does
not impose any restrictions on the location of AWS credentials/config settings and just relies on
the AWS SDK to find credentials and authenticate. If you are ensure of hw this should be done we 
recommend doing either of the following: 
- `aws sso login`, and `export AWS_PROFILE` environment variable.
- export AWS_ACCESS_KEY_ID=... export AWS_SECRET_ACCESS_KEY=... export AWS_SESSION_TOKEN=...`
environmental variables

In order to exit the application, simply Ctrl+C in the terminal.

### Running using the Docker container

The docker app requires the following environment variables:

- `AWS_ACCESS_KEY_ID`, `AWS_SECRET_ACCESS_KEY` and `AWS_SESSION_TOKEN`
- optional `TRANSCRIPT_DESTINATION` - this can either be the URL for which the transcribed data
  will be submitted using HTTP POST method or the Pubnub channel if it is used.

Run as:

```
docker run -p 8080:8080 -e AWS_ACCESS_KEY_ID=... -e AWS_SECRET_ACCESS_KEY=... -e AWS_SESSION_TOKEN=... transcription
```

The Docker app does not automatically start the transcription, but it exposes
the following REST API endpoints on port 8080 for controlling its tasks:

- `/health`
- `/ready`
- `/terminate`
- `/start`
- `/stop`

The `/start` endpoint can be used to start the transcription. The query params
are:

- `alias` or `id` - for conference alias or conference ID
- `token` - the CAPI access token
- `username` - the user name
- `service` - The transcription service to be used: `gladia` (default) or `aws`
- `gladiakey` - The Gladia.io API key, must be provided if using gladia service.
- `sendmechanism` - The mechanism used to send out transcripts. This can be either `pubnub` or `http`.
- `publishkey` - The publish key for Pubnub if it is used as send mechanism.
- `subscribekey` - The subscribe key for Pubnub if it is used as send mechanism.

The `/stop` or `/terminate` endpoint can be used to terminate the application 
stop transcribing.

### General hints for running the RTME

The application supports transcribing a single conference at a time.

The transcription starts as soon as the application joins the conference, and
if the conference ends, the application does not quit automatically - it will
just keep the session open with no more activity. The integrator should know
when does the conference start, and when does it end, in order to be able to
spawn the transcription when there's such demand, and to shut it down when not
needed any more.

The application does not support any form of token refresh. It should be spawned
with the Dolby.io authorisation token with such an expiration time, that it will
be sufficient to stay in the conference until the conference ends.

## Modifying the transcription component

The transcription component allows for easy modification of the three
subcomponents: the logger, the transcription service used, and the way the
transcribed text is consumed by other components in the system.

### Modifying the logger

The `src/interfaces/logger.h` contains the definition of the logger class. The
corresponding `.cc` file contains the implementation of the `logger::make()`
method, which returns the instance of the logger. Any custom logger
implementation can be easily injected into the transcription module by
reimplementing that method to return the instance of your own class which needs
to extend the logger interface.

### Modifying the transcription service

The `src/interfaces/transcription_service.h` file contains the transcription
interface. Just like with the logger, the instance of the transcription service
can be replaced by reimplementing the `transcription_service::create()`
function.

### Modifying the transcription sink

The transcripts are passed to the conference as conference messages.
Applications wishing to implement their own handling of the transcriptions
should modify the `src/sample_code_plugin.cc` file to pass their own
implementation of the interface defined in
`src/interfaces/transcription_listener.h` file.
