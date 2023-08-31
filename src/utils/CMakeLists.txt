add_library(rtme_utils STATIC
	async_accumulator.h
	async_accumulator.cc
	logger.h
	logger.cc
	processing_thread.h
	processing_thread.cc
	signal_handler.h
	signal_handler.cc
	wrap_move_only_object.h	
)

set_target_properties(rtme_utils
	PROPERTIES POSITION_INDEPENDENT_CODE ON
)

target_link_libraries(rtme_utils PRIVATE
	Threads::Threads
	DolbyioComms::sdk
)

target_include_directories(rtme_utils PRIVATE
	${RTME_INCLUDE_DIR}
	${DOLBYIO_COMMS_SDK_ROOT_DIR}/share
)

target_include_directories(rtme_utils PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
	$<INSTALL_INTERFACE:include>
)
