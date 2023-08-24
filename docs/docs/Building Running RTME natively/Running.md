#Running RTME Natively

## Running directly from build / build_docker_dir

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

## General hints for running the RTME

The application supports transcribing a single conference at a time.

The transcription starts as soon as the application joins the conference, and
if the conference ends, the application does not quit automatically - it will
just keep the session open with no more activity. The integrator should know 
when does the conference start, and when does it end, in order to be able to 
spawn the transcription when there's such demand, and to shut it down when not 
needed any more.

The application does not support any form of token refresh. It should be spawned 
with the Dolby.io authorisation token with such an expiration time, that it will 
be sufficient to stay in the conference until the conference end
