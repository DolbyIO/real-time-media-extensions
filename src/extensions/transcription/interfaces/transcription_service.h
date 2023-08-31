#pragma once

#include "speech_to_text/buffer/audio_ringbuffer.h"
#include "utils/logger.h"

#include <dolbyio/comms/sdk.h>

#include <map>
#include <optional>

namespace dolbyio::comms::rtme::transcription {

class transcription_service {
 public:
  class text_listener {
   public:
    virtual void is_ready(std::string&& speech,
                          bool finalized,
                          size_t begin_timestamp,
                          size_t end_timestamp,
                          std::string unique_id = {}) = 0;
    virtual void error(std::string&& description) = 0;
  };

  struct config {
    text_listener* listener_{nullptr};
    logger& logger_;
    sdk* sdk_{nullptr};
    std::map<std::string, std::string> engine_params_{};
  };

  // Implement your own custom factory for implementing your own custom
  // transcription backend:
  class factory {
   public:
    virtual ~factory() = default;

    virtual std::string name() const = 0;
    // param, description:
    virtual std::map<std::string, std::string> custom_params() const = 0;
    virtual std::unique_ptr<transcription_service> create(
        config&& config) const = 0;
  };

  transcription_service(text_listener* listener, logger& logger);
  virtual ~transcription_service();

  virtual audio_ringbuffer<float>& get_buffer() = 0;

  virtual void start() = 0;
  virtual void stop() = 0;

 protected:
  logger& logger_;
  text_listener* listener_{nullptr};
};
}  // namespace dolbyio::comms::rtme::transcription
