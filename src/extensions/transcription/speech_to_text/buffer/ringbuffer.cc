#include "speech_to_text/buffer/ringbuffer.h"

#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>

#include <cassert>
#include <stdexcept>

namespace dolbyio::comms::rtme::transcription {

namespace {
struct scoped_fd {
  int fd_;
  scoped_fd(int fd) : fd_(fd) {}
  ~scoped_fd() {
    if (fd_ >= 0)
      close(fd_);
  }
};
}  // namespace

ringbuffer::ringbuffer(size_t size_bytes) : size_(size_bytes) {
  if (size_ % 4096 != 0)
    throw std::runtime_error("Ringbuffer size must be page-aligned");
  int fd = memfd_create("", 0);
  if (fd < 0)
    throw std::runtime_error("Can't allocate memfd");
  scoped_fd close_on_return(fd);
  if (ftruncate(fd, size_) != 0)
    throw std::runtime_error("Can't resize memory region");

  void* addr =
      mmap(nullptr, size_ * 2, PROT_NONE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
  if (addr == MAP_FAILED)
    throw std::runtime_error("Can't reserve address");

  char* buf1_ptr = reinterpret_cast<char*>(addr);
  char* buf2_ptr = buf1_ptr + size_;

  void* buf1 = mmap(buf1_ptr, size_, PROT_READ | PROT_WRITE,
                    MAP_FIXED | MAP_SHARED, fd, 0);
  void* buf2 = mmap(buf2_ptr, size_, PROT_READ | PROT_WRITE,
                    MAP_FIXED | MAP_SHARED, fd, 0);
  if (buf1 == MAP_FAILED || buf2 == MAP_FAILED) {
    munmap(buf1_ptr, size_);
    munmap(buf2_ptr, size_);
    throw std::runtime_error("Can't map ringbuffer once");
  }
  assert(buf1 == buf1_ptr);
  buffer_ = buf1_ptr;
}

ringbuffer::~ringbuffer() {
  if (buffer_) {
    munmap(buffer_, size_ * 2);
  }
}

void ringbuffer::reset_pointers() {
  read_ptr_ = 0;
  write_ptr_ = 0;
}

void* ringbuffer::read(size_t size) {
  if (size > size_)
    throw std::invalid_argument("Buffer overflow");
  (void)write_ptr_.load(std::memory_order_acquire);
  auto* retval = buffer_ + (read_ptr_ % size_);
  read_ptr_ += size;
  return retval;
}

void* ringbuffer::read_no_overflow(size_t& size) {
  if (size > size_)
    throw std::invalid_argument("Buffer overflow");
  size_t cur_write_ptr = write_ptr_.load(std::memory_order_acquire);

  size_t avail = cur_write_ptr > read_ptr_ ? cur_write_ptr - read_ptr_ : 0;
  size = std::min(size, avail);
  auto* retval = buffer_ + (read_ptr_ % size_);
  read_ptr_ += size;
  return retval;
}

void ringbuffer::write(void* data, size_t size) {
  void* dest = write_begin(size);
  memcpy(dest, data, size);
  write_end(size);
}

void* ringbuffer::write_begin(size_t size) {
  if (size > size_)
    throw std::invalid_argument("Buffer overflow");
  auto* dest = buffer_ + (write_ptr_.load(std::memory_order_relaxed) % size_);
  return dest;
}

void ringbuffer::write_end(size_t size) {
  if (size > size_)
    throw std::invalid_argument("Buffer overflow");
  write_ptr_.fetch_add(size, std::memory_order_release);
}

}  // namespace dolbyio::comms::rtme::transcription
