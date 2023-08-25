---
title: Gladia.io
id: gladia-io
---

You can access the Gladia.io service via the WebSocket connection responsible for sending audio data and receiving transcriptions. To connect to Gladia.io, follow these steps:

1. Create a [Gladia.io](https://app.gladia.io/auth/signup) account using your email and password or your Google account.
2. Find your API key in the Gladia.io dashboard.
3. Pass the following query parameters using the `/start` REST endpoint:
    - `service=gladia`
    - `gladiakey=API_KEY_FROM_GLADIAIO`
