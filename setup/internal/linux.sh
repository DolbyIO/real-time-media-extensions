#!/bin/bash

set -e
set -x

WORK_DIR=$(pwd)

source setup/internal/dolbyiosdk_internal.sh
source setup/awssdk.sh
source setup/transcription.sh

function show_help() {
	echo " Usage:"
	echo "      bash set/linux.sh [--sdk_version SDK_VERSION_TO_USE]"
	echo
	echo " Simple example which will use defaults:"
	echo "      bash setup/linux.sh"
	echo
	echo " --sdk_version      	The commit hash of the package you want to fetch. If not set the top level hash of your current branch will be used."
	echo " --build_docker       Build the docker image."
	echo " -h|--help          	Print this help."
}

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
	mkdir $WORK_DIR/ext-lib/
fi
pushd $WORK_DIR/ext-lib/

# Parse command line arguments
while [[ $# -gt 0 ]]; do
  case $1 in
		--sdk_version)
			sdk_version="$2"
			shift
			shift
			;;
		--build_docker)
			build_docker=1
			shift
			;;
		-h|--help)
			show_help
			exit 0
			;;
		*)
			loge_and_exit "Unknown option: $1"
			;;
  esac
done

# Get the version of the SDK to be used
if [ -z ${sdk_version} ]; then
	sdk_version=$(cat "${WORK_DIR}/setup/sdk_version.txt")
fi
logi "SDK package of the following commit: ${sdk_version} will be used."

# Fetch SDK package if hashes do not match
if should_fetch_new_sdk; then
	# Fetch the sdk-package and unzip it 
	fetch_sdk
	unzip_sdk_package
else
	logi "Not fetching new SDK packages as hashes match"
fi

if aws_sdk_is_present; then
	logi "AWS transcribe package is already present!"
else
	logi "Clonging and compiling AWS SDK package!"
	clone_compile_aws_sdk
fi 

# Build the transcription application anre preare the
# docker image.
build_cpp_transcription
prepare_docker_image

# Build the docker image only if specified.
if [[ $build_docker == 1 ]]; then
	echo "Building docker image"
	build_docker_image
fi
