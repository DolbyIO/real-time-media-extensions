#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace dolbyio::comms::rtme::transcription {
class vad_bitmap {
  using word_t = uint32_t;
  constexpr static size_t bits_in_word = sizeof(word_t) * 8;
  constexpr static word_t mask_lut[bits_in_word] = {
      0xffffffff >> 0,  0xffffffff >> 1,  0xffffffff >> 2,  0xffffffff >> 3,
      0xffffffff >> 4,  0xffffffff >> 5,  0xffffffff >> 6,  0xffffffff >> 7,
      0xffffffff >> 8,  0xffffffff >> 9,  0xffffffff >> 10, 0xffffffff >> 11,
      0xffffffff >> 12, 0xffffffff >> 13, 0xffffffff >> 14, 0xffffffff >> 15,
      0xffffffff >> 16, 0xffffffff >> 17, 0xffffffff >> 18, 0xffffffff >> 19,
      0xffffffff >> 20, 0xffffffff >> 21, 0xffffffff >> 22, 0xffffffff >> 23,
      0xffffffff >> 24, 0xffffffff >> 25, 0xffffffff >> 26, 0xffffffff >> 27,
      0xffffffff >> 28, 0xffffffff >> 29, 0xffffffff >> 30, 0xffffffff >> 31};

 public:
  vad_bitmap(int num_samples);
  ~vad_bitmap();

  void set_vad(size_t sample_start, size_t num_samples, bool vad);
  bool get_vad(size_t sample_start, size_t num_samples) const;

 private:
  const size_t num_samples_;
  std::vector<word_t> bitmap_;
};
}  // namespace dolbyio::comms::rtme::transcription
