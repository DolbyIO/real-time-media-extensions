---
title: Live Transcription
id: live-transcription
---

The Live Transcription application is designed to be seamlessly integrated with the Communications APIs conferencing platform and add live transcription to your virtual meetings. The application uses speech recognition to provide instant and accurate transcriptions of spoken content, which enhances accessibility, engagement, collaboration, and overall meeting effectiveness.

The application supports transcription in one conference at a time. The transcription starts as soon as the application joins a conference. When the conference ends, the application does not quit automatically; it keeps the session open without any activity. 

The application does not support any form of token refresh. We recommend using the Dolby.io authorization token with a sufficient expiration time to stay in a conference until the end.

For transcription, our application supports [AWS Transcribe](https://aws.amazon.com/transcribe/) and [Gladia.io](https://www.gladia.io/) services. The following guides explain how to configure the RTME runtime to connect to the preferred service:

- [AWS Transcribe](./Live%20Transcription/Aws-transcribe.md)
- [Gladia.io](./Live%20Transcription/Gladia-io.md)