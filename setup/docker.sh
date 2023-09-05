install_dir=${WORK_DIR}/install_dir

build_cpp_rtme() {
	build_dir=${WORK_DIR}/build_dir
	if [ ! -d ${build_dir} ]; then
		mkdir -p ${build_dir}
	fi
	pushd ${build_dir}

	cmake -GNinja ../ -DCMAKE_BUILD_TYPE=RelWithDebInfo
	cmake --build .
	cmake --install . --prefix ${install_dir}
	popd
}

prepare_docker_image() {
	docker_dir=${WORK_DIR}/build_docker_dir
	if [ ! -d ${docker_dir}/docker/resources ]; then
		mkdir -p ${docker_dir}/docker/resources
	fi
	pushd ${docker_dir}
	cp -av ${install_dir}/* docker/resources/
	if [ ! -d docker/resources/bin ]; then
		mkdir -p docker/resources/bin
	fi
	cp -av ${WORK_DIR}/ext-lib/sdk-release-ubuntu-20.04-gcc-gnustl/bin/* docker/resources/bin
	cp -av ${WORK_DIR}/ext-lib/sdk-release-ubuntu-20.04-gcc-gnustl/lib/* docker/resources/lib
	popd
}

build_docker_image() {
	docker_repo=${1}
	docker_tag=${2}
	pushd ${WORK_DIR}
	docker build -t ${docker_repo}:${docker_tag} -f Dockerfile --no-cache .
	docker save --output dolbyio_rtme-${docker_tag}.tar ${docker_repo}:${docker_tag}
	gzip -9 dolbyio_rtme-${docker_tag}.tar
	popd
}
