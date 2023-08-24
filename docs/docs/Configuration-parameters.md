---
title: Configuration Parameters
id: configuration-parameters
---

The following configuration parameters are essential for RTME applications. You can provide them to a container image and a Linux executable.

## General
| Item       | Meaning                                                 | Linux executable argument | Container parameter | Possible values                     |
|------------|---------------------------------------------------------|---------------------------|---------------------|-------------------------------------|
| **Plugin** | The full path to the application plugin, in .so format. | `-plugin`                 | N/A                 | `libdolbyio_comms_transcription.so` |


## Communications

| Item            | Meaning                                                                                                                                                                                                                                                                                                                         | Linux executable argument | Container parameter to the `/start` endpoint |
|-----------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|---------------------------|----------------------------------------------|
| **Alias**       | The Communications API conference [alias](https://docs.dolby.io/communications-apis/docs/guides-creating-a-conference) RTME runtime should connect to. When a conference with a specific alias does not exist, using the alias results in creating the conference. Therefore, we recommend using alias only for local testing.  | `-c`                      | `alias`                                      |
| **ID**          | The Communications API conference [ID](https://docs.dolby.io/communications-apis/docs/guides-creating-a-conference) RTME runtime should connect to.                                                                                                                                                                             | `-i`                      | `id`                                         |
| **Token**       | The Communications APIs [client access token](https://docs.dolby.io/communications-apis/docs/guides-client-authentication) required to connect to a conference. RTME does not support any form of token refresh, therefore, we recommend using a token with a sufficient expiration time to stay in a conference until the end. | `-k`                      | `client-token`                               |
| **User name**   | The user name representing the RTME runtime in the Communications APIs conference.                                                                                                                                                                                                                                              | `-u`                      | `username`                                   |
| **Listen-only** | Information whether RTME should be run in the listen-only mode. The mode excludes the possibility to inject media into a conference.                                                                                                                                                                                            | `-m none`                 | N/A                                          |

<!-- Currently unsupported

### Real-time Streaming
| Item  | Meaning  | Linux executable argument| Container parameter |
|-----------|----------|-----------|----------|
| **Subscribe token**  | The Real-time streaming subscribe token.  | | 
| **Publish token**  | The Real-time streaming publish token (reserved for future use).  |
| **Stream**  | The Real-time streaming stream name, in the format of `accountId`/`streamName`.  |

-->

## Mechanisms for sending RTME data
RTME offers mechanisms for transferring data processed by the application outside of the docker container in real time. You can specify the preferred mechanism by providing the proper query parameter to the /start REST endpoint. When it is not specified, the RTME application stores data in a file.

| Item                | Meaning                                                                                                      | QueryParam to /start REST endpoint | Possible Values   |
|---------------------|--------------------------------------------------------------------------------------------------------------|------------------------------------|-------------------|
| **Send Mechanism**  | The mechanism used to send data from outside the RTME application. Currently this can be `http` or `pubnub`. | sendmechanism                      | `http`, `pubnub`  |

### PubNub
Using PubNub as the send mechanism requires PubNub credentials that you can access after [registering](https://admin.pubnub.com/#/register) a PubNub account.
| Item                | Meaning                          | QueryParam to /start REST endpoint | Possible Values   |
|---------------------|----------------------------------|--------------------------------------------------------|
| **Publish key**     | The API Key used for publishing. | publishkey                         | String            |
| **Subscribe key**   | The API Key used for subscribing.| subscribekey                       | String            |

### Data endpoint
| Item                       | Meaning                                                                                                                                                                                                                                                                                    | Environment variable     |
|----------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|--------------------------|
| **Transcript destination** | The destination where transcribed data should be submitted. When using the `http` sendmechanism this destination is the URL where data can be sent with HTTP POST method. When using the `pubnub` sendmechanism this destination is the PubNub channel where the transcripts will be sent. | `TRANSCRIPT_DESTINATION` |

## Application-specific configurations

### Live transcription

| Item                        | Meaning                                                                                                                     | Linux executable               | /start REST endpoint QueryParam | Possible Values   |
|-----------------------------|-----------------------------------------------------------------------------------------------------------------------------|--------------------------------|---------------------------------|-------------------|
| **Transcription provider**  | The transcription service to use.                                                                                           | `--rtme-transcription-service` | service                         | `aws` or `gladia` |
| **Transcription parameter** | The service-specific parameters to use.                                                                                     | `--rtme-transcription-param`   | N/A                             | `param1:value1`   |
| **Gladia API key**          | The [API key](https://docs.gladia.io/reference/overview#getting-your-api-key) required to connect to the Gladia.io service. | `api-key`                      | gladiakey                       | N/A               |

To use the AWS service, such as AWS Transcribe, the RTME runtime must have appropriate AWS credentials. Furthermore, the authenticated AWS role should have access to the AWS Transcribe service. RTME does not dictate the location of the AWS credentials/config settings; it relies on the AWS SDK for authentication. If you are unsure about this process, consider using either of the following [environment variables](https://docs.aws.amazon.com/cli/latest/userguide/cli-configure-envvars.html):

- `aws sso login` and `export AWS_PROFILE` if you use a Linux executable
- `export AWS_ACCESS_KEY_ID=...`, `export AWS_SECRET_ACCESS_KEY=...`, and ` export AWS_SESSION_TOKEN=...`

| Item                       | Meaning                                                                                                                                                                                                           | Environment variable     |
|----------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-------------------------|
| **AWS profile**            | The name of the AWS Command Line Interface (CLI) profile with the credentials and options to use.                                                                                                                 | `AWS_PROFILE`           |
| **AWS access key**         | A unique identifier associated with an AWS Identity and Access Management (IAM) user or role. It is a part of the security credentials for authenticating your applications and services with AWS.                | `AWS_ACCESS_KEY_ID`     |
| **AWS secret access key**  | The password paired with the access key that securely signs requests to AWS. It must be kept confidential to protect your resources.                                                                              | `AWS_SECRET_ACCESS_KEY` |
| **AWS session token**      | A temporary token used along with the access key and secret key in AWS when multi-factor authentication (MFA) is enabled or when roles are assumed. This helps to ensure the security and integrity of your data. | `AWS_SESSION_TOKEN`     |

