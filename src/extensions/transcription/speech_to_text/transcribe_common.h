#pragma once

#include "speech_to_text/buffer/audio_ringbuffer.h"

#include <atomic>
#include <functional>
#include <stddef.h>

namespace dolbyio::comms::rtme::transcription {
class transcribe_common {
 protected:
  transcribe_common(int sample_rate,
                    uint8_t channels,
                    uint8_t total_size_seconds);

  void read_audio_loop(size_t sample_size_ms,
                       size_t samples_in_second,
                       std::function<void(float*, size_t)>&& transcribe_audio);
  bool audio_contains_voice(bool vad);
  size_t timestamp_to_num_samples(const double& timestamp_seconds);
  size_t adjust_timestamp_for_silence(size_t curr_timestamp);

  virtual bool exit_read_audio_loop() = 0;

  audio_ringbuffer<float> ringbuffer_;
  size_t current_read_ptr_{0};
  std::atomic<bool> transcript_finalized_{false};
 private:
  size_t samples_not_sent_{0};
  size_t continuous_silence_counter_{0};
};

}  // namespace dolbyio::comms::rtme::transcription
