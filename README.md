[![Build Realtime Media Extensions](https://github.com/DolbyIO/real-time-media-extensions/actions/workflows/build.yml/badge.svg)](https://github.com/DolbyIO/real-time-media-extensions/actions/workflows/build.yml)
[![Documentation](https://github.com/DolbyIO/real-time-media-extensions/actions/workflows/deploy-to-github-pages.yml/badge.svg)](https://github.com/DolbyIO/real-time-media-extensions/actions/workflows/deploy-to-github-pages.yml)
[![License](https://img.shields.io/github/license/DolbyIO/real-time-media-extensions)](LICENSE)

##### Table of Contents
- [Running RTME docker](#runningdocker)
- [Building RTME Plugin](#buildplugin)
- [Running RTME natively](#runningnatively)

# Dolby.io Realtime Media Extensions

The Dolby.io Realtime Media Extensions provide a complete solution for
automated, realtime media processing in the Dolby.io infrastructure. Each
component provides its own API, but the project as a whole provides also the
ready-to-use application, as well as the Docker image and Kubernetes pod
definition.

The RTME application can be run directly on the Ubuntu host (build machine) or
as a docker image. Running from the docker container exposes some REST API
endpoints, while running directly allows for quick iteration times.

<a name="runningdocker"/>

## Running using the Docker container
The docker app requires the following environment variables:

- (if using AWS transcribe) `AWS_ACCESS_KEY_ID`, `AWS_SECRET_ACCESS_KEY` and `AWS_SESSION_TOKEN`
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

<a name="buildplugin"/>

## Building
If you would like to build the sources from scratch and/or modify the components please pages for [building](LINK TO Building.md) and [modifying components](LINK TO Modifying.md).

<a name="runningnatively"/>

## Running natively
If you would like to run the application natively on a Ubuntu 20.04 please refer to the [following pages](LINK TO Running.md).
