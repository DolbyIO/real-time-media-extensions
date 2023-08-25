## Building

### Build system requirements and prerequisites

- Ubuntu 20.04
- CMake 3.21

After unpacking the source, you should install the latest C++ SDK release for
Ubuntu 20.04, with GNU ABI, in the ext-lib/sdk-release-ubuntu-20.04-gcc-gnustl
in the unpacked source directory. This is the only manual step, the build script
will handle installing other dependencies and building the project.

### Building the Ubuntu 20.04 application and all components

Run the `setup/build.sh` script. It will download and build other dependencies,
like the AWS SDK. The script will create three directories:

- `build_dir` (the CMake build directory)
- `install_dir` (the installation directory of the project)
- `build_docker_dir` (the pre-packaging directory for creating a docker
  image)

The contents of the `install_dir` contain the build artifacts, dependencies and
public headers of the RTME project, but it does not contain the C++ SDK
libraries or the desktop_app. The `build_docker_dir` contains everything.

In result of a successful build, the `transcription` docker image is created and
the tarball containing it is compressed in the `build_docker_dir`.

### Building directly from CMake

If the `setup/build.sh` script has been run at least once, all required
dependencies in the `ext-lib` directory are prepared. At this point, it's
possible to build the RTME project directly from CMake with no custom config
params required:

```
	cmake -GNinja <RTME_source_directory> -DCMAKE_BUILD_TYPE=RelWithDebInfo
	cmake --build .
```

The RTME project can be included as a subproject in another CMake project, and
the installed artifacts contain CMake module definitions for easily importing
prebuilt components into your CMake project.


