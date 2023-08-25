---
sidebar_label: Linux executable
title: Install and run RTME as a Linux executable
id: linux-executable
---
Before you start, make sure that you use Ubuntu 20.04 or later.

## Download RTME from GitHub
To download an RTME Linux executable, open the [GitHub releases](https://github.com/voxeet/rtme/releases) page, find the preferred version, and click the package that you wish to download.

## Run RTME directly from build or build_docker_dir
You can directly launch the RTME runtime from the Linux command line interface. 
This is useful for quick experiments and testing of the RTME. Depending on the application,
you will need to supply appropriate [configuration](../configuration) parameters. 

```
desktop_app [command line arguments] 
```

Based on the configuration, the RTME runtime will join a conference or connect to a Real-time
Streaming stream and start processing media to, for example, forward the audio to one of the supported
live transcription providers.

In order to exit the application, use Ctrl+C in the terminal.
