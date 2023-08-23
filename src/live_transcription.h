#pragma once

#ifndef __linux__
#error "The live transcription addon only supports linux for now"
#endif  // !linux

#include <dolbyio/comms/media_engine/media_engine.h>
#include <dolbyio/comms/sdk.h>

#include "interfaces/transcription_listener.h"
#include "interfaces/transcription_service.h"

#include <map>

#ifndef TRANSCRIPTION_EXPORT
#define TRANSCRIPTION_EXPORT __attribute__((visibility("default")))
#endif  // TRANSCRIPTION_EXPORT

namespace dolbyio::comms::transcription {
class TRANSCRIPTION_EXPORT live_transcription
    : public dolbyio::comms::mixed_audio_sink {
 public:
  struct config {
    std::optional<log_level> logging_level{};
    bool use_vad{false};
    std::map<std::string, std::string> engine_params{};
  };
  virtual ~live_transcription();

  static async_result<std::unique_ptr<live_transcription>> create(
      dolbyio::comms::sdk& sdk,
      const config&& config,
      const std::shared_ptr<transcription_service::factory>& service_factory,
      const std::shared_ptr<transcription_listener>& listener);
};
}  // namespace dolbyio::comms::transcription
