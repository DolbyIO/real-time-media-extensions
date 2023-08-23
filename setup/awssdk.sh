clone_compile_aws_sdk() {
	pushd ${WORK_DIR}/ext-lib
	if [ ! -d aws-sdk-install ]; then
		mkdir aws-sdk-install
	fi
	if [ ! -d aws-sdk-cpp ]; then
		git clone https://github.com/aws/aws-sdk-cpp.git
	fi
	pushd aws-sdk-cpp
	git checkout ${AWS_SDK_VERSION}
	git submodule update --init --recursive
	if [ ! -d build ]; then
		mkdir build
	fi
	cd build
	cmake -GNinja ../ -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=${WORK_DIR}/ext-lib/aws-sdk-install -DBUILD_ONLY="transcribestreaming" -DCPP_STANDARD=17 -DENABLE_TESTING=OFF
	cmake --build .
	cmake --install .
	popd
	echo ${AWS_SDK_VERSION} > aws-sdk-install/version.txt
	popd
}

aws_sdk_is_present() {
	need_to_fetch="yes"
	pushd ${WORK_DIR}/ext-lib
	if [ -f aws-sdk-install/version.txt ]; then
		aws_hash=$(cat aws-sdk-install/version.txt)
		if [[ $aws_hash == "${AWS_SDK_VERSION}" ]]; then
			need_to_fetch="no"
		else
			rm -rf aws-sdk-install/
		fi
	fi
	popd
	if [[ ${need_to_fetch} == "yes" ]]; then
		return 1
	fi
	return 0
}
