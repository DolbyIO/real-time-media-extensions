---
sidebar_label: Docker container
title: Install and run RTME as a Docker container
id: docker-container
---

[Docker](https://www.docker.com/whatisdocker) is a tool that can package an application and its dependencies in a virtual container to run on different host operating systems.

## Get RTME from Dockerhub

To pull the latest version of RTME, use the following command:

```shell
$ docker pull dolbyio/rtme
```

You can also pull a specific version, for example, v0.1.1. The full list of versions is available on
[Dockerhub](https://hub.docker.com/r/dolbyio/rtme/tags/).

```shell
$ docker pull dolbyio/rtme:release-v0.1.1
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
| `/start`     | Sends a command with query parameters to the RTME docker runtime to start the application. See the [Configuration](../configuration) section for more information.                                                                                                                                                    |
| `/stop`      | Stops the current RTME application.                                                                                                                                                                                                                                                                                   |