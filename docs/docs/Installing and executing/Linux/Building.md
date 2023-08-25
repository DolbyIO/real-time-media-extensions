---
sidebar_label: Building RTME
title: Building RTME
id: building
---

### Requirements and prerequisites

- Ubuntu 20.04
- CMake 3.21

After unpacking the source, install the latest [C++ SDK release](https://github.com/DolbyIO/comms-sdk-cpp/releases) for
Ubuntu 20.04 with GNU ABI in `ext-lib/sdk-release-ubuntu-20.04-gcc-gnustl`
in the unpacked source directory. This is the only manual step, the build script
will handle installing other dependencies and building the project.

### Building the Ubuntu 20.04 application and all components

Run the `setup/build.sh` script to download and build other dependencies,
such as the AWS SDK. The script creates three directories:

- `build_dir`: The CMake build directory
- `install_dir`: The installation directory of the project
- `build_docker_dir`: The pre-packaging directory for creating a docker
  image

The `install_dir` directory contains the build artifacts, dependencies, and
public headers of the RTME project; it does not contain the C++ SDK
libraries or the desktop application. The `build_docker_dir` directory contains everything.

After a successful build, the `transcription` docker image is created and
the tarball, which contains it, is compressed in the `build_docker_dir`.

### Building directly from CMake

If the `setup/build.sh` script has been run at least once, all required
dependencies in the `ext-lib` directory are prepared. At this point, it is
possible to build the RTME project directly from CMake with no custom config
params required, as in the following example:

```
	cmake -GNinja <RTME_source_directory> -DCMAKE_BUILD_TYPE=RelWithDebInfo
	cmake --build .
```

The RTME project can be included as a subproject in another CMake project. 
The installed artifacts contain CMake module definitions to simplify importing
prebuilt components into your CMake project.


