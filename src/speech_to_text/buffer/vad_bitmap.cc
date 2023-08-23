#include "speech_to_text/buffer/vad_bitmap.h"

namespace dolbyio::comms::transcription {

vad_bitmap::vad_bitmap(int num_samples)
    : num_samples_(num_samples),
      bitmap_((num_samples + bits_in_word - 1) / bits_in_word, 0) {}

void vad_bitmap::set_vad(size_t sample_start, size_t num_samples, bool vad) {
  if (sample_start + num_samples > num_samples_) {
    set_vad(sample_start, num_samples_ - sample_start, vad);
    set_vad(0, num_samples + sample_start - num_samples_, vad);
    return;
  }
  size_t index_start = sample_start / bits_in_word;
  size_t index_end = (sample_start + num_samples) / bits_in_word;

  if (vad) {
    bitmap_[index_start] |= mask_lut[sample_start % bits_in_word];
  } else {
    bitmap_[index_start] &= ~mask_lut[sample_start % bits_in_word];
  }

  if (index_end == index_start)
    return;

  for (size_t i = index_start + 1; i < index_end; ++i) {
    bitmap_[i] = vad ? ~0x0 : 0x0;
  }

  size_t endbit = index_end % bits_in_word;
  if (endbit == 0)
    return;  // the loop handled it

  if (vad) {
    bitmap_[index_end] |= ~mask_lut[endbit];
  } else {
    bitmap_[index_end] &= mask_lut[endbit];
  }
}

bool vad_bitmap::get_vad(size_t sample_start, size_t num_samples) const {
  if (num_samples == 0)
    return false;
  if (sample_start + num_samples > num_samples_) {
    return get_vad(sample_start, num_samples_ - sample_start) ||
           get_vad(0, num_samples + sample_start - num_samples_);
  }
  size_t index_start = sample_start / bits_in_word;
  size_t index_end = (sample_start + num_samples) / bits_in_word;

  if (bitmap_[index_start] & mask_lut[sample_start % bits_in_word])
    return true;

  for (size_t i = index_start + 1; i < index_end; ++i) {
    if (bitmap_[i])
      return true;
  }

  size_t endbit = index_end % bits_in_word;
  if (endbit != 0) {
    return bitmap_[index_end] & ~mask_lut[endbit];
  }

  return false;
}

vad_bitmap::~vad_bitmap() = default;

}  // namespace dolbyio::comms::transcription
