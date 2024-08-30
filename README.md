# :warning: This repository is no longer maintained :warning:

[![Build Realtime Media Extensions](https://github.com/DolbyIO/real-time-media-extensions/actions/workflows/build.yml/badge.svg)](https://github.com/DolbyIO/real-time-media-extensions/actions/workflows/build.yml)
[![Documentation](https://github.com/DolbyIO/real-time-media-extensions/actions/workflows/deploy-to-github-pages.yml/badge.svg)](https://github.com/DolbyIO/real-time-media-extensions/actions/workflows/deploy-to-github-pages.yml)
[![License](https://img.shields.io/github/license/DolbyIO/real-time-media-extensions)](LICENSE)

##### Table of Contents
- [Running RTME docker](#runningdocker)
- [Building RTME plugin](#buildplugin)
- [Running RTME natively](#runningnatively)

# Dolby.io Real-time Media Extensions

The Dolby.io Real-time Media Extensions (RTME) provide a complete solution for
automated, real-time media processing in the Dolby.io infrastructure. Each
component provides its own API, but the project as a whole provides also the
ready-to-use application, as well as the Docker image and Kubernetes pod
definition.

The RTME application can be run directly on the Ubuntu host (build machine) or
as a docker image. Running from the docker container exposes some REST API
endpoints, while running directly allows for quick iteration times. 
Installation procedures are available in the [documentation](https://api-references.dolby.io/real-time-media-extensions/docs/introduction).

<a name="runningdocker"/>

## Running using the Docker container
The docker application requires the following environment variables:

- `AWS_ACCESS_KEY_ID`, `AWS_SECRET_ACCESS_KEY` and `AWS_SESSION_TOKEN` if using AWS transcribe
- Optional `TRANSCRIPT_DESTINATION` that can either be the URL for which the transcribed data
  will be submitted using the HTTP POST method or the PubNub channel.

Run the application as in the following example:

```
docker run -p 8080:8080 -e AWS_ACCESS_KEY_ID=... -e AWS_SECRET_ACCESS_KEY=... -e AWS_SESSION_TOKEN=... transcription
```

The Docker application does not automatically start the transcription. It exposes
the following REST API endpoints on port 8080 for controlling its tasks:

- `/health`
- `/ready`
- `/terminate`
- `/start`
- `/stop`

The `/start` endpoint can be used to start the transcription. The query parameters
are:

| Item            | Meaning                                                                                                                                                                                                                                                                                                                         |
|-----------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `alias`         | The Communications API conference [alias](https://docs.dolby.io/communications-apis/docs/guides-creating-a-conference) RTME runtime should connect to. When a conference with a specific alias does not exist, using the alias results in creating the conference. Therefore, we recommend using alias only for local testing.  |
| `id`            | The Communications API conference [ID](https://docs.dolby.io/communications-apis/docs/guides-creating-a-conference) RTME runtime should connect to.                                                                                                                                                                             |
| `token`         | The Communications APIs [client access token](https://docs.dolby.io/communications-apis/docs/guides-client-authentication) required to connect to a conference. RTME does not support any form of token refresh, therefore, we recommend using a token with a sufficient expiration time to stay in a conference until the end. |
| `username`      | The user name representing the RTME runtime in the Communications APIs conference.                                                                                                                                                                                                                                              |
| `service`       | The transcription service to use, either `aws` or `gladia`                                                                                                                                                                                                                                                                      |
| `gladiakey`     | The Gladia.io API key.                                                                                                                                                                                                                                                                                                          |
| `sendmechanism` | The mechanism used to send data from outside the RTME application, either `http` or `pubnub`                                                                                                                                                                                                                                    |
| `publishkey`    | The publish key for PubNub if it is used as send mechanism.                                                                                                                                                                                                                                                                     |
| `subscribekey`  | The subscribe key for PubNub if it is used as send mechanism.                                                                                                                                                                                                                                                                   |

Use the `/stop` or `/terminate` endpoint to terminate the application or 
stop transcribing.

For more information, see the RTME [documentation](https://api-references.dolby.io/real-time-media-extensions/docs/Installing%20and%20executing/docker-container).

<a name="buildplugin"/>

## Building
If you would like to build the sources from scratch and/or modify the components, see the [Building](https://api-references.dolby.io/real-time-media-extensions/docs/Installing%20and%20executing/Linux/building) and [Modifying Components](https://api-references.dolby.io/real-time-media-extensions/docs/Installing%20and%20executing/Linux/modifying-components) instructions.

<a name="runningnatively"/>

## Running natively
If you would like to run the application natively on Ubuntu 20.04, refer to the [instruction](https://api-references.dolby.io/real-time-media-extensions/docs/Installing%20and%20executing/Linux/running-natively).
