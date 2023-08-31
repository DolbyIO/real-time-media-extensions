#include "interfaces/transcription_service.h"

#include "speech_to_text/aws/aws_transcribe.h"

#include <iostream>

namespace dolbyio::comms::rtme::transcription {

transcription_service::transcription_service(text_listener* listener,
                                             logger& logger)
    : listener_(listener), logger_(logger) {}

transcription_service::~transcription_service() = default;

}  // namespace dolbyio::comms::rtme::transcription
