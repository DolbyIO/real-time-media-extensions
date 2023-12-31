---
sidebar_label: Docker container
title: Installing and running RTME as a Docker container
id: docker-container
---

[Docker](https://www.docker.com/whatisdocker) is a tool that can package an application and its dependencies in a virtual container to run on different host operating systems.

## Get RTME from GitHub Container Registry 

To pull the latest version of RTME, use the following command:

```shell
$ docker pull ghcr.io/dolbyio/real-time-media-extensions:latest
```

You can also pull a specific version, for example, v0.1.1. The full list of versions is available on
[GitHub Container Registry](https://github.com/dolbyio/real-time-media-extensions/pkgs/container/real-time-media-extensions).

```shell
$ docker pull ghcr.io/dolbyio/real-time-media-extensions:1.0.0
```

## Run the container

To run the container, use the following command:

```shell
docker run -p 8080:8080 -e {optional environment variables} dolbyio/rtme
```

Depending on the application, you may need to supply certain environment variables to the `docker run` command. 

The Docker container does not automatically start the RTME service, but it exposes
the following REST API endpoints on port 8080 for controlling its tasks. These endpoints map to generic [Kubernetes](https://kubernetes.io/) liveness, readiness, and startup [probes](https://kubernetes.io/docs/tasks/configure-pod-container/configure-liveness-readiness-startup-probes/), making your application easier to manage within a Kubernetes environment. 

| Endpoint     | Meaning                                                                                                                                                                                                                                                                                                               |
|--------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `/health`    | Kubernetes uses liveness probes to know when to restart a container. For example, liveness probes could catch a deadlock while an application is running, but are unable to make progress. Restarting a container in such a state can help to make the application more available despite bugs.                       |
| `/ready`     | Kubernetes uses readiness probes to know when a container is ready to start accepting traffic. A Pod is considered ready when all of its containers are ready. One use of this signal is to control which Pods are used as backends for services. When a Pod is not ready, it is removed from service load balancers. |
| `/terminate` | Terminates the RTME docker runtime.                                                                                                                                                                                                                                                                                   |
| `/start`     | Sends a command with query parameters to the RTME docker runtime to start the application. See the [Configuration](../Configuration-parameters.md) section for more information.                                                                                                                                         |
| `/stop`      | Stops the current RTME application.                                                                                                                                                                                                                                                                                   |
## CA Certificates

The docker image uses the standard CA certificates that come with the base Ubuntu 20.04 image. If you need different certificates you can update the certificates in the image as follows:

- Add the following to the Dockerfile and rebuild the image
```
ADD YOUR-CA-CERTIFICATES.crt /usr/local/share/ca-certificates/additional-ca.crt
RUN chmod 644 /usr/local/share/ca-certificates/additional-ca.crt && update-ca-certificates
```

- Run the docker image providing the newly updated certificates (these certificates will be used for this run)
```
docker run -p 8080:8080 -v update-ca-certificates.crt:/etc/ssl/certs/ca-certificates.crt -d dolbyio/rtme
```
