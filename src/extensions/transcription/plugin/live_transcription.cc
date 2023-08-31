#include "plugin/live_transcription.h"

#include "utils/logger.h"
#include "speech_to_text/speech_to_text.h"
#include "utils/wrap_move_only_object.h"

namespace dolbyio::comms::rtme::transcription {

namespace {
async_result<std::shared_ptr<dolbyio::comms::logger_sink>>
register_sdk_component(dolbyio::comms::sdk& sdk) {
  return sdk
      .register_component_version(
          "dolbyio-realtime-media-extensions-transcription",
          DOLBYIO_TRANSCRIPTION_VERSION)
      .then([](auto&& data) { return std::move(data.logger); });
}
}  // namespace

live_transcription::~live_transcription() = default;

async_result<std::unique_ptr<live_transcription>> live_transcription::create(
    sdk& sdk,
    const config&& config,
    const std::shared_ptr<transcription_service::factory>& service_factory,
    const std::shared_ptr<transcription_listener>& listener) {
  return register_sdk_component(sdk).then(
      [&sdk, config{std::move(config)}, listener{listener}, service_factory](
          std::shared_ptr<dolbyio::comms::logger_sink>&& logger) mutable {
        if (config.logging_level)
          logger->set_level(*config.logging_level);
        auto stt = std::make_unique<speech_to_text>(
            std::move(listener), logger::make(logger), sdk, *service_factory,
            std::move(config));
        auto res = stt->initialize();
        return std::move(res).then(
            [stt{wrap_move_only_object(std::move(stt))}]() mutable
            -> std::unique_ptr<live_transcription> { return std::move(*stt); });
      });
}

}  // namespace dolbyio::comms::rtme::transcription
