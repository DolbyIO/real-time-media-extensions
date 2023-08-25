package messenger

import (
  "bytes"
  "encoding/json"
  "io/ioutil"
  "log"
  "net/http"

  "github.com/pubnub/go/v7"
)

type MessageSender interface {
  Configure(key string, value string)
  Initialize()
  SendMessage(data []byte, dest string)
  Shutdown()
}

type pubnub_sender struct {
  uuid string
  config *pubnub.Config
  pn *pubnub.PubNub
}

func pubnub_sender_constructor(uuid string) *pubnub_sender {
  pbs := &pubnub_sender{}
  pbs.config = pubnub.NewConfigWithUserId(pubnub.UserId(uuid))
  return pbs
}

func (ps *pubnub_sender) Configure(key string, value string) {
  switch key {
    case "publish":
      ps.config.PublishKey = value
    case "subscribe":
      ps.config.SubscribeKey = value
  }
}

func (ps *pubnub_sender) Initialize() {
  ps.pn = pubnub.NewPubNub(ps.config)
}

func jsonToInterfaceMap(data []byte) map[string]interface{} {
  jsonMap := make(map[string]interface{})
  json.Unmarshal(data, &jsonMap)
  return jsonMap
}

func (ps *pubnub_sender) SendMessage(data []byte, dest string) {
  _, stat, _ := ps.pn.Publish().Channel(dest).Message(jsonToInterfaceMap(data)).Execute()
  log.Println("Sending the pubnub message result:", stat.Operation, "code:", stat.StatusCode)
}

func (ps *pubnub_sender) Shutdown() {}

type http_sender struct {
  client *http.Client
}

func (hs *http_sender) SendMessage(data []byte, dest string) {
 request, err := http.NewRequest("POST", dest, bytes.NewBuffer(data))
  if err != nil {
    log.Println("Error creating the send transcript request:", err)
    return
  }
  request.Header.Set("Content-Type", "application/json")
  response, err := hs.client.Do(request)
  if err != nil {
    log.Println("Error executing the send transcript request:", err)
    return
  }
  defer response.Body.Close()
  response_data, err := ioutil.ReadAll(response.Body)
  if err != nil {
    log.Println("Error reading the send transcript request:", err)
    return
  }
  log.Println("Response data: ", string(response_data))
}

func (hs *http_sender) Configure(key string, value string) {}

func (hs *http_sender) Initialize() {
  hs.client = &http.Client{}
}

func (hs *http_sender) Shutdown() {}

func Create(val string, uuid string) MessageSender {
  log.Println("Calling create", val)
  if val == "pubnub" {
    return pubnub_sender_constructor(uuid)
  } else {
    return &http_sender{}
  }
}
