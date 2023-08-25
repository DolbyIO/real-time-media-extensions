#pragma once

#include "interfaces/logger.h"
#include "interfaces/transcription_service.h"
#include "live_transcription.h"
#include "speech_to_text/buffer/audio_ringbuffer.h"
#include "speech_to_text/conference_tracker.h"
#include "speech_to_text/aws/json_value.h"

#include <dolbyio/comms/logger_sink.h>
#include <dolbyio/comms/media_engine/audio_utils.h>

#include <array>
#include <cassert>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <time.h>

namespace dolbyio::comms {
class sdk;
}

namespace dolbyio::comms::transcription {

class transcript_timepoint {
 public:
  struct time_point {
    struct tm* utc_time;
    size_t utc_millis;
  };
  operator bool() const { return initialized_; }
  void initialize();
  time_point timestamp_to_utc(size_t timestamp_ms);

 private:
  std::time_t start_time_seconds_;
  size_t start_time_milliseconds_;
  struct tm tm_buffer_;
  bool initialized_{false};
};

class speech_to_text : public transcription_service::text_listener,
                       public live_transcription {
  static constexpr int g_input_channels = 2;
  static constexpr int g_input_rate = 48000;
  static constexpr int g_output_channels = 1;

 public:
  speech_to_text(std::shared_ptr<transcription_listener>&& listener,
                 std::shared_ptr<logger>&& logger,
                 dolbyio::comms::sdk& sdk,
                 const transcription_service::factory& service_factory,
                 live_transcription::config&& cfg);
  ~speech_to_text() override;

  void handle_audio(const int16_t* data,
                    size_t n_data,
                    int sample_rate,
                    size_t channels) override;
  async_result<void> initialize();

 private:
  void process_and_store(const int16_t* data,
                         int sample_rate,
                         int channels,
                         int num_data);
  void is_ready(std::string&& speech,
                bool finalized,
                size_t begin_timestamp,
                size_t end_timestamp,
                std::string unique_id) override;
  void error(std::string&& description) override;
  std::string jsonisze_transcript(
      const transcription_listener::transcript& transcript,
      std::chrono::milliseconds start_latency);
  std::string numsamples_to_utc_timestamp(size_t samples);

  sdk& sdk_;
  std::shared_ptr<logger> logger_;
  std::shared_ptr<transcription_listener> listener_;
  std::unique_ptr<transcription_service> transcription_wrapper_;
  audio_ringbuffer<float>& ringbuffer_;
  std::unique_ptr<conference_tracker> conf_tracker_;
  std::unique_ptr<audio_utils::audio_converter> converter_{};
  std::unique_ptr<audio_utils::voice_activity_detector> vad_{};
  json_value json_;
  struct previous_transcript {
    bool compare(const std::string& t, bool f) {
      auto same_text = (text.compare(t) == 0);
      if ((finalized == f) && same_text) {
        return true;
      }
      if (!same_text) {
        text = t;
      }
      finalized = f;
      return false;
    }
    std::string text;
    bool finalized{false};
  };
  previous_transcript previous_transcript_;
  transcript_timepoint start_time_point_;
};

}  // namespace dolbyio::comms::transcription
