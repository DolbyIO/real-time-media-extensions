################################################################################
# Build:
# docker build -t transcription .
#
# Run (OSX):
# docker run --name transcription \
#      -it --rm \
#      <arguments>....
#
################################################################################

FROM ubuntu:20.04

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get --quiet update --yes \
  && apt-get --quiet dist-upgrade --yes \
  && apt-get --quiet install --yes --fix-missing -o 'Acquire::Retries=40' curl libxcomposite1 libxdamage1 libxfixes3 libxrandr2 libxtst6 libcurl4 \
  && rm -rf /var/lib/apt/lists/*
COPY build_docker_dir/docker/resources /opt/transcription/

ENTRYPOINT ["/opt/transcription/bin/transcription"]
