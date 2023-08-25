---
sidebar_label: Running RTME Natively
title: Running RTME Natively
id: running-natively
---

Before running RTME, build it or download the RTME Linux executable from [GitHub](https://github.com/DolbyIO/real-time-media-extensions/releases).

## Run RTME directly from build or build_docker_dir

You can directly launch the RTME runtime from the Linux command line interface. The application requires providing command line
parameters and application-specific configurations described in the [Configuration Parameters](../Configuration-parameters.md) document.

The RTME project provides the `libdolbyio_comms_transcription.so` plugin. Locate
it in the build directory and run the `desktop_app` using command line arguments, as in the following example:

```
desktop_app -k <token> -u test_user -c <conference_alias> --plugin <path_to_dolbyio_transcription> --rtme-transcription-service <transcription_service> -m none --rtme-transcription-param param1:value1
```

To use the Gladia.io transcription service the plugin requires an API key in order
to connect to the Gladia.io service. This api key must be provided as follows: 
`--rtme-transcription-parameter api-key:VALUE`.

To use AWS service, ensure that proper AWS credentials are accessible by the plugin. After authentication, make sure 
the AWS role has access to the AWS Transcribe service. The plugin itself does
not impose any restrictions on the location of AWS credentials/config settings and just relies on
the AWS SDK to find credentials and authenticate. If you are unsure of how to do it, we 
recommend using either of the following environment variables: 
- `aws sso login` and `export AWS_PROFILE`
- `export AWS_ACCESS_KEY_ID=... export AWS_SECRET_ACCESS_KEY=... export AWS_SESSION_TOKEN=...`

After running, the RTME runtime joins a conference and starts processing media to, for example, forward audio to one of the supported live transcription providers. To exit the application, use Ctrl+C in the terminal.

The application supports transcribing a single conference at a time. The transcription starts as soon as the application joins the conference. When the conference ends, the application does not quit automatically; it keeps the session open with no more activity. The integrator should know when the conference starts and ends in order to spawn the transcription when there is such demand and to shut it down when needed.

The application does not support any form of token refresh. It should be spawned 
with the Dolby.io authorization token with an expiration time that is 
sufficient to stay in the conference until the end of the conference.