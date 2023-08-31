#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <vector>  // for VAD bitmap. refactor!

namespace dolbyio::comms::rtme::transcription {
// single producer, single consumer
class ringbuffer {
 public:
  ringbuffer(size_t size_bytes);
  ~ringbuffer();

  void reset_pointers();
  size_t size() const { return size_; }
  void* read(size_t size);
  void* read_no_overflow(size_t& size);
  void write(void* data, size_t size);
  void* write_begin(size_t size);
  void write_end(size_t size);
  size_t get_read_ptr() const { return read_ptr_; }
  size_t get_write_ptr() const {
    return write_ptr_.load(std::memory_order_acquire);
  }

 private:
  const size_t size_;
  char* buffer_ = nullptr;

  alignas(64) size_t read_ptr_{0};
  alignas(64) std::atomic<size_t> write_ptr_{0};
};

}  // namespace dolbyio::comms::rtme::transcription
