#pragma once

#ifndef __linux__
#error "The RTME extensions only support linux for now"
#endif  // !linux

#ifndef DOLBYIO_COMMS_RTME_EXPORT
#define DOLBYIO_COMMS_RTME_EXPORT __attribute__((visibility("default")))
#endif  // TRANSCRIPTION_EXPORT

#if defined(DOLBYIO_COMMS_RTME_TRANSCRIPTION)
#include "extensions/transcription/plugin/interactor.h"
#endif
