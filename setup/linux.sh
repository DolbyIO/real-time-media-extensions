#!/bin/bash

set -e
set -x

WORK_DIR=$(pwd)

source setup/sdks/dolbyiosdk.sh
source setup/sdks/awssdk.sh
source setup/docker.sh

function show_help() {
	echo " Usage:"
	echo "      bash setup/linux.sh [--dolby_sdk_version SDK_VERSION_TO_USE]"
	echo
	echo " Simple example which will use defaults:"
	echo "      bash setup/linux.sh"
	echo
	echo " --dolby_sdk_version  The commit hash of the Dolby.io package you want to fetch. If not set the version in the sdks/version_dolbyio.txt will be used.."
	echo " --build_docker       Build the docker image with the specified tag and name for the image. The image will be dolby/rtme-TAG."
	echo " --skip_building     	Skip building the dependencies, c++ plugin and golang binary. This should be used if docker resources exist."
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
		--dolby_sdk_version)
			sdk_version="$2"
			shift
			shift
			;;
		--build_docker)
			docker_repo="$2"
			docker_tag="$3"
			shift
			shift
			shift
			;;
		--skip_building)
			skip_building=1
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

# If specified skip the building of resources
if [[ -z ${skip_building} ]]; then
	# Get the version of the SDK to be used
	if [ -z ${sdk_version} ]; then
		sdk_version=$(cat "${WORK_DIR}/setup/sdks/version_dolbyio.txt")
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

	# Build the RTME applications and prepare the
	# docker image.
	build_cpp_rtme
	prepare_docker_image
fi

# Build the docker image only if specified.
if [[ ! -z ${docker_repo} ]]; then
	echo "Building docker image"
	build_docker_image ${docker_repo} ${docker_tag}
fi
