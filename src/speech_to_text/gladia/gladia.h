#pragma once

#include "interfaces/transcription_service.h"
#include "speech_to_text/processing_thread.h"
#include "speech_to_text/transcribe_common.h"

#include <aws/core/utils/base64/Base64.h>
#include "dolbyio/comms/utils/netio.h"

namespace dolbyio::comms::transcription {

class websocket_listener
    : public dolbyio::comms::utils::netio::websocket::listener {
 public:
  class callback {
   public:
    enum class status { connected, closed };
    virtual void on_data(std::string_view data) = 0;
    void on_status_changed(status stat) {
      std::lock_guard<std::mutex> lock(lock_);
      status_ = stat;
    }
   protected:
    bool websocket_connected() {
      std::lock_guard<std::mutex> lock(lock_);
      return status_ == status::connected;
    }
   private:
    std::mutex lock_;
    status status_{status::connected};
  };
  void on_read(std::string_view data) override;
  void on_closed(bool clean_shutdown) override;
  void on_reconnected() override;

  websocket_listener(callback* callback) : callback_(callback) {}
  ~websocket_listener() override {}

 private:
  callback* callback_{nullptr};
};

class transcript_timestamp_tracker {
 public:
  transcript_timestamp_tracker(size_t estimated_latency);
  void transcript_arrived(size_t curr_time, size_t duration);
  size_t calculate_end_time(size_t duration);
  void reset_state();

  size_t last_start_time() const { return last_start_time_; }
  size_t average_start_time() const { return avg_start_time_; }
  const std::string& transcript_id() const { return transcript_id_str_; }
 private:
  const size_t estimate_latency_{0};
  size_t average_counter_{0};
  size_t last_start_time_{0};
  size_t avg_start_time_{0};
  size_t transcript_id_{1};
  std::string transcript_id_str_;
};

class gladia : public transcription_service,
               public processing_thread,
               public transcribe_common,
               public websocket_listener::callback {
 public:
  gladia(transcription_service::config&& cfg);
  ~gladia() override;
  static std::shared_ptr<factory> factory();
  audio_ringbuffer<float>& get_buffer() override { return ringbuffer_; }
  void thread_function() override;
  void start() override;
  void stop() override;
  void on_data(std::string_view data) override;

 private:
  bool exit_read_audio_loop() override;
  std::string prepare_gladia_data(float* data, size_t num_samples);

  std::map<std::string, std::string> params_{};
  Aws::Utils::Base64::Base64 base64_{};
  std::shared_ptr<websocket_listener> ws_listener_{};
  std::unique_ptr<dolbyio::comms::utils::netio::websocket> ws_{};
  transcript_timestamp_tracker timestamp_tracker_;
  size_t continuous_websocket_send_failures_{0};
};

}  // namespace dolbyio::comms::transcription
