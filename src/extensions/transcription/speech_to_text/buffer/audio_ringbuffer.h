#pragma once

#include "speech_to_text/buffer/ringbuffer.h"
#include "speech_to_text/buffer/vad_bitmap.h"

namespace dolbyio::comms::rtme::transcription {
class audio_ringbuffer_base {
 public:
  audio_ringbuffer_base(size_t data_type_size,
                        int sample_rate,
                        int num_channels,
                        std::chrono::milliseconds min_size);
  ~audio_ringbuffer_base();

  // size_t size_samples() const { return buf_.size() / sample_size_; }
  size_t sample_size() const { return sample_size_; }
  size_t sample_rate() const { return sample_rate_; }
  size_t channels() const { return channels_; }
  size_t get_read_ptr() const { return buf_.get_read_ptr() / sample_size_; }
  size_t get_write_ptr() const { return buf_.get_write_ptr() / sample_size_; }

  void reset_pointers() { buf_.reset_pointers(); }
  void* read(size_t num_samples, bool* vad);
  void* read_no_overflow(size_t& num_samples, bool* vad);
  void write(void* data, size_t num_samples, bool vad);
  void* write_begin(size_t num_samples);
  void write_end(size_t num_samples, bool vad);

 private:
  static size_t calculate_bufsize(size_t data_type_size,
                                  int sample_rate,
                                  int num_channels,
                                  std::chrono::milliseconds min_size);

  ringbuffer buf_;
  const size_t sample_size_;
  const size_t sample_rate_;
  const size_t channels_;
  vad_bitmap bitmap_;
};

template <typename sample_type_t>
class audio_ringbuffer : public audio_ringbuffer_base {
 public:
  audio_ringbuffer(int sample_rate,
                   int num_channels,
                   std::chrono::milliseconds min_size)
      : audio_ringbuffer_base(sizeof(sample_type_t),
                              sample_rate,
                              num_channels,
                              min_size) {}

  sample_type_t* read(size_t num_samples, bool* vad) {
    return reinterpret_cast<sample_type_t*>(
        audio_ringbuffer_base::read(num_samples, vad));
  }

  sample_type_t* read_no_overflow(size_t& num_samples, bool* vad) {
    return reinterpret_cast<sample_type_t*>(
        audio_ringbuffer_base::read_no_overflow(num_samples, vad));
  }

  sample_type_t* write_begin(size_t num_samples) {
    return reinterpret_cast<sample_type_t*>(
        audio_ringbuffer_base::write_begin(num_samples));
  }

  void write_end(size_t num_samples, bool vad) {
    return audio_ringbuffer_base::write_end(num_samples, vad);
  }
};
}  // namespace dolbyio::comms::rtme::transcription
