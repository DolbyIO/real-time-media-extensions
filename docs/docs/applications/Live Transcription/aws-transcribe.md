---
title: AWS Transcribe
id: aws-transcribe
---

Before using the AWS service, ensure that proper credentials for AWS are accessible by the plugin. Also, make sure the authenticated AWS role has access to the AWS Transcribe service. The plugin itself does not impose any restrictions on the location of AWS credentials/config settings and relies on the AWS SDK to find credentials and authenticate. If you are unsure of how to do it, we recommend using either of the following environment variables:

- `aws sso login, and export AWS_PROFILE`
- `export AWS_ACCESS_KEY_ID=... export AWS_SECRET_ACCESS_KEY=... export AWS_SESSION_TOKEN=...`

When starting the RTME application via the `/start` REST endpoint, pass the `service=aws` query parameter.