#include "speech_to_text/buffer/audio_ringbuffer.h"

namespace dolbyio::comms::rtme::transcription {
audio_ringbuffer_base::audio_ringbuffer_base(size_t data_type_size,
                                             int sample_rate,
                                             int num_channels,
                                             std::chrono::milliseconds min_size)
    : buf_(calculate_bufsize(data_type_size,
                             sample_rate,
                             num_channels,
                             min_size)),
      sample_size_(num_channels * data_type_size),
      sample_rate_(sample_rate),
      channels_(num_channels),
      bitmap_(buf_.size() / sample_size_) {}

audio_ringbuffer_base::~audio_ringbuffer_base() = default;

void* audio_ringbuffer_base::read(size_t num_samples, bool* vad) {
  size_t sample_idx = (buf_.get_read_ptr() % buf_.size()) / sample_size_;
  void* ret = buf_.read(num_samples * sample_size_);
  if (vad)
    *vad = bitmap_.get_vad(sample_idx, num_samples);
  return ret;
}

void* audio_ringbuffer_base::read_no_overflow(size_t& num_samples, bool* vad) {
  size_t sample_idx = (buf_.get_read_ptr() % buf_.size()) / sample_size_;
  size_t bytes = num_samples * sample_size_;
  auto* ret = buf_.read_no_overflow(bytes);
  num_samples = bytes / sample_size_;
  if (vad)
    *vad = bitmap_.get_vad(sample_idx, num_samples);
  return ret;
}

void audio_ringbuffer_base::write(void* data, size_t num_samples, bool vad) {
  size_t sample_idx = (buf_.get_write_ptr() % buf_.size()) / sample_size_;
  bitmap_.set_vad(sample_idx, num_samples, vad);
  buf_.write(data, num_samples * sample_size_);
}

void* audio_ringbuffer_base::write_begin(size_t num_samples) {
  return buf_.write_begin(num_samples * sample_size_);
}

void audio_ringbuffer_base::write_end(size_t num_samples, bool vad) {
  size_t sample_idx = (buf_.get_write_ptr() % buf_.size()) / sample_size_;
  bitmap_.set_vad(sample_idx, num_samples, vad);
  return buf_.write_end(num_samples * sample_size_);
}

size_t audio_ringbuffer_base::calculate_bufsize(
    size_t data_type_size,
    int sample_rate,
    int num_channels,
    std::chrono::milliseconds min_size) {
  static constexpr const size_t ms_in_second = 1000;
  static constexpr const size_t page_size = 4096;
  const size_t sample_size = data_type_size * num_channels;
  const size_t num_samples_times_1000 = sample_rate * min_size.count();
  const size_t num_samples =
      (sample_rate * min_size.count() + ms_in_second - 1) / ms_in_second;
  const size_t num_pages =
      (num_samples * sample_size + page_size - 1) / page_size;
  return num_pages * page_size;
}
}  // namespace dolbyio::comms::rtme::transcription
