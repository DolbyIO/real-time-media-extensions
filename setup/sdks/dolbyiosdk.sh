should_fetch_new_sdk() {
    if [ -f sdk-release-ubuntu-20.04-gcc-gnustl/version.txt ]; then
        current_version=$(cat sdk-release-ubuntu-20.04-gcc-gnustl/version.txt)
    else
        current_version="none"
    fi

    if [ "$current_version" != "$sdk_version" ]; then
        # If we want to use a different package clear the old one
        if [ "$current_version" != "none" ]; then
            rm -rf sdk-release-ubuntu-20.04-gcc-gnustl/
        fi

        true
        return
    else
        false
        return
    fi
}

fetch_sdk() {
	# Check after the first attempt if the hash exists
	curl https://github.com/DolbyIO/comms-sdk-cpp/releases/download/${sdk_version}/cppsdk-${sdk_version}-ubuntu-20.04-gcc-gnustl.zip -O -J -L
	if [ ! -f cppsdk-${sdk_version}-ubuntu-20.04-gcc-gnustl.zip ]; then
		loge_and_exit "The commit ${sdk_version} does not exist"
	fi
}

unzip_sdk_package() {
	if [ -z cppsdk-${sdk_version}-ubuntu-20.04-gcc-gnustl.zip ]; then
		loge_and_exit "There is not sdk zip package here!"
	fi

	unzip -q cppsdk-${sdk_version}-ubuntu-20.04-gcc-gnustl.zip
	rm *.zip
	echo ${sdk_version} > sdk-release-ubuntu-20.04-gcc-gnustl/version.txt
}
