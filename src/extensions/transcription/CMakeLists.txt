add_library(transcription SHARED
	${RTME_INCLUDE_DIR}/plugin/rtme_plugin_type.h
	${RTME_INCLUDE_DIR}/plugin/rtme_plugin.cc
	plugin/interactor.h
	plugin/interactor.cc
	plugin/live_transcription.h
	plugin/live_transcription.cc
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
	speech_to_text/speech_to_text.h
	speech_to_text/speech_to_text.cc
	speech_to_text/transcribe_common.h
	speech_to_text/transcribe_common.cc
	speech_to_text/file_writer.h
	speech_to_text/file_writer.cc
)

target_include_directories(transcription PRIVATE
	${RTME_INCLUDE_DIR}
	${DOLBYIO_COMMS_SDK_ROOT_DIR}/share
)

target_include_directories(transcription PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
	$<INSTALL_INTERFACE:include>
)

target_link_libraries(transcription PRIVATE 
	DolbyioComms::sdk
	DolbyioComms::media
	rtme_utils
)

target_compile_definitions(transcription PRIVATE
	DOLBYIO_COMMS_RTME_TRANSCRIPTION	
	DOLBYIO_COMMS_ENABLE_DEPRECATED_WAIT
	DOLBYIO_TRANSCRIPTION_VERSION="${CMAKE_PROJECT_VERSION}"
)

target_link_libraries(transcription PRIVATE
	aws-sdk-cpp
)

set_target_properties(transcription PROPERTIES
	PUBLIC_HEADER plugin/live_transcription.h
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
	EXPORT DolbyioCommsRtmeTranscription
	PUBLIC_HEADER
		DESTINATION include/dolbyio/comms/rtme/transcription
)
