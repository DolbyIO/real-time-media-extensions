#!/bin/bash

set -e
set -x

WORK_DIR=$(pwd)

source setup/awssdk.sh
source setup/docker.sh

function logi() {
  local msg=$1
  printf "INFO: %s\n" "$msg"
}

function loge() {
  local msg=$1
  printf "ERROR: %s\n" "$msg"
}

function loge_and_exit() {
  local msg=$1
  local err_code=${2:-1} # If the error code is not given as a second argument then return 1 by default
  loge "$msg"
  exit $err_code
}

# Enter ext-lib to place all the dependencies
if [ ! -d $WORK_DIR/ext-lib ]; then
    loge_and_exit "You need to put the C++ SDK in ext-lib directory"
fi

logi "Script is running locally"
source ${WORK_DIR}/setup/local_defaults.sh

if aws_sdk_is_present; then
	logi "AWS transcribe package is already present!"
else
	logi "Cloning and compiling AWS SDK package!"
	clone_compile_aws_sdk
fi 

build_cpp_transcription
prepare_docker_image
build_docker_image
