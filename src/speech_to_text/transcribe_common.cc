#include "speech_to_text/transcribe_common.h"

#include <thread>

namespace dolbyio::comms::transcription {
transcribe_common::transcribe_common(int sample_rate,
                                     uint8_t channels,
                                     uint8_t total_size_seconds)
    : ringbuffer_(sample_rate,
                  channels,
                  std::chrono::seconds(total_size_seconds)) {}

void transcribe_common::read_audio_loop(
    size_t sample_size_ms,
    size_t samples_in_second,
    std::function<void(float*, size_t)>&& transcribe_audio_cb) {
  size_t num_samples_in_chunk = ringbuffer_.sample_rate() / samples_in_second;
  while (!exit_read_audio_loop()) {
    bool vad = false;
    size_t num_samples = num_samples_in_chunk;
    current_read_ptr_ = ringbuffer_.get_read_ptr();
    size_t total_samples = num_samples;
    float* samples = ringbuffer_.read_no_overflow(total_samples, &vad);
    if (!audio_contains_voice(vad)) {
      if (total_samples == 0) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(sample_size_ms / 2));
      }
      samples_not_sent_ += total_samples;
      continue;
    }
    while (total_samples * 1000 / ringbuffer_.sample_rate() < sample_size_ms) {
      std::chrono::milliseconds sleep_time{
          sample_size_ms - (total_samples * 1000 / ringbuffer_.sample_rate())};
      std::this_thread::sleep_for(sleep_time);
      num_samples = num_samples_in_chunk - total_samples;
      ringbuffer_.read_no_overflow(num_samples, &vad);
      total_samples += num_samples;
      if (exit_read_audio_loop()) {
        return;
      }
    }
    if (audio_contains_voice(vad)) {
      transcribe_audio_cb(samples, total_samples);
    } else {
      samples_not_sent_ += total_samples;
    }
  }
}

bool transcribe_common::audio_contains_voice(bool vad) {
  bool should_send = false;
  if (vad) {
    should_send = true;
    continuous_silence_counter_ = 0;
  } else if (!transcript_finalized_) {
    if (++continuous_silence_counter_ <= 5) {
      should_send = true;
    }
  }
  return should_send;
}

size_t transcribe_common::timestamp_to_num_samples(
    const double& timestamp_sec) {
  auto timestamp_ms = timestamp_sec * 1000;
  size_t num_samples = ringbuffer_.sample_rate() / 1000 * timestamp_ms;
  return num_samples;
}

size_t transcribe_common::adjust_timestamp_for_silence(size_t curr_timestamp) {
  return curr_timestamp + samples_not_sent_;
}

}  // namespace dolbyio::comms::transcription
