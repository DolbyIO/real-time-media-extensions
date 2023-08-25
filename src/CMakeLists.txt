add_library(transcription SHARED
	sample_code_plugin.cc
	live_transcription.h
	live_transcription.cc
	interfaces/logger.h
	interfaces/logger.cc
	interfaces/transcription_service.h
	interfaces/transcription_service.cc
	interfaces/transcription_listener.h
	interfaces/transcription_listener.cc
	speech_to_text/aws/aws_transcribe.h
	speech_to_text/aws/aws_transcribe.cc
	speech_to_text/aws/json_value.h
	speech_to_text/aws/json_value.cc
	speech_to_text/gladia/gladia.h
	speech_to_text/gladia/gladia.cc
	speech_to_text/buffer/audio_ringbuffer.h
	speech_to_text/buffer/audio_ringbuffer.cc
	speech_to_text/buffer/ringbuffer.h
	speech_to_text/buffer/ringbuffer.cc
	speech_to_text/buffer/vad_bitmap.h
	speech_to_text/buffer/vad_bitmap.cc
	speech_to_text/conference_tracker.h
	speech_to_text/conference_tracker.cc
	speech_to_text/processing_thread.h
	speech_to_text/processing_thread.cc
	speech_to_text/speech_to_text.h
	speech_to_text/speech_to_text.cc
	speech_to_text/transcribe_common.h
	speech_to_text/transcribe_common.cc
	utils/async_accumulator.h
	utils/async_accumulator.cc
	utils/file_writer.h
	utils/file_writer.cc
	utils/logger.h
	utils/signal_handler.h
	utils/signal_handler.cc
	utils/wrap_move_only_object.h
)

target_include_directories(transcription PRIVATE
	${DOLBYIO_COMMS_SDK_ROOT_DIR}/share
)

target_include_directories(transcription PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
	$<INSTALL_INTERFACE:include>
)

target_link_libraries(transcription PUBLIC
	Threads::Threads
	DolbyioComms::sdk
	DolbyioComms::media
)

target_compile_definitions(transcription PRIVATE
    DOLBYIO_COMMS_ENABLE_DEPRECATED_WAIT
    DOLBYIO_TRANSCRIPTION_VERSION="${CMAKE_PROJECT_VERSION}"
)

target_link_libraries(transcription PRIVATE
	aws-sdk-cpp
)

set_target_properties(transcription PROPERTIES
	PUBLIC_HEADER live_transcription.h
	OUTPUT_NAME dolbyio_comms_transcription
)

dump_debug_symbols_and_strip(transcription)
install_debuginfo(transcription lib)

get_target_property(AWS_CORE_LIBRARY aws-cpp-sdk-core IMPORTED_LOCATION)
get_target_property(AWS_TS_LIBRARY aws-cpp-sdk-transcribestreaming IMPORTED_LOCATION)
install(FILES
	${AWS_CORE_LIBRARY} ${AWS_TS_LIBRARY}
	DESTINATION lib
)

install(TARGETS transcription
	EXPORT DolbyioCommsTranscription
	PUBLIC_HEADER
		DESTINATION include/dolbyio/comms/transcription
)

include(CMakePackageConfigHelpers)

install(EXPORT DolbyioCommsTranscription
	NAMESPACE DolbyioCommsTranscription::
	FILE DolbyioCommsTranscription.cmake
	DESTINATION share/dolbyio/comms/cmake
)

set(DOLBYIO_COMMS_CMAKE_PKG_NAME DolbyioCommsTranscription)
configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/package_config.cmake.in
	"${CMAKE_CURRENT_BINARY_DIR}/DolbyioCommsTranscriptionConfig.cmake"
	INSTALL_DESTINATION share/dolbyio/comms/cmake
)

write_basic_package_version_file(
	"${CMAKE_CURRENT_BINARY_DIR}/DolbyioCommsTranscriptionConfigVersion.cmake"
	VERSION "${PROJECT_VERSION}"
	COMPATIBILITY ExactVersion
)

install(FILES
	"${CMAKE_CURRENT_BINARY_DIR}/DolbyioCommsTranscriptionConfig.cmake"
	"${CMAKE_CURRENT_BINARY_DIR}/DolbyioCommsTranscriptionConfigVersion.cmake"
	DESTINATION share/dolbyio/comms/cmake
)

add_subdirectory(pulse_audio)

find_program(GO_EXEC go REQUIRED
	PATHS /usr/local/go/bin/
)
include(ExternalProject)
ExternalProject_Add(transcription_service
	SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../
	DOWNLOAD_COMMAND ""
	INSTALL_COMMAND ""
	CONFIGURE_COMMAND ""
	BUILD_COMMAND ${GO_EXEC} build -o transcription .
	COMMAND mv transcription ${CMAKE_CURRENT_BINARY_DIR}
	BUILD_IN_SOURCE True
	BUILD_BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/transcription"
	BUILD_ALWAYS True
)

install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/transcription DESTINATION bin)
