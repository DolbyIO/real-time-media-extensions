set(MODULE_NAME pulse_audio)
get_filename_component(MODULE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/${MODULE_NAME}" DIRECTORY)

set(HEADER_DIR "${MODULE_ROOT}/include/${MODULE_NAME}")
set(SOURCE_DIR "${MODULE_ROOT}/src")

add_library(pulse_audio SHARED
    ${SOURCE_DIR}/pulse_audio.cpp
)

find_package(PulseAudio)

if(NOT PULSEAUDIO_FOUND)
    message(FATAL_ERROR "Failed to find PulseAudio")
endif()

target_include_directories(pulse_audio PRIVATE
  "${MODULE_ROOT}/include"
  "${PULSE_INCLUDE_DIR}"
)

set_target_properties(pulse_audio PROPERTIES
    SOVERSION 0
    OUTPUT_NAME pulse
    LINK_FLAGS "-Wl,--version-script=${SOURCE_DIR}/pulse_audio.map"
)

install(TARGETS pulse_audio
	EXPORT DolbyioCommsTranscription
	PUBLIC_HEADER
		DESTINATION include/dolbyio/comms/transcription
)

# add_subdirectory(test)
