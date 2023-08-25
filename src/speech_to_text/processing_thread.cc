#include "processing_thread.h"

#include <cassert>

namespace dolbyio::comms::transcription {

semaphore::semaphore(int count, size_t max_count)
    : count_(count), max_count_(max_count) {}

semaphore::~semaphore() {
  {
    std::lock_guard<std::mutex> lock(lock_);
    count_ = max_count_;
  }
  cond_.notify_all();
}

void semaphore::post() {
  {
    std::lock_guard<std::mutex> lock(lock_);
    if (++count_ && (count_ > max_count_))
      count_ = max_count_;
  }
  cond_.notify_one();
}

bool semaphore::wait_timeout(std::chrono::milliseconds&& timeout) {
  bool finished_waiting = true;
  std::unique_lock<std::mutex> lock(lock_);
  if (count_ <= 0) {
    finished_waiting = cond_.wait_for(lock, std::move(timeout),
                                      [this]() { return count_ > 0; });
  }
  if (finished_waiting)
    --count_;
  return finished_waiting;
}

void semaphore::wait() {
  std::unique_lock<std::mutex> lock(lock_);
  if (count_ <= 0) {
    cond_.wait(lock, [this]() { return count_ > 0; });
  }
  --count_;
}

processing_thread::processing_thread() = default;

processing_thread::~processing_thread() {
  assert(thread_ == nullptr);
}

void processing_thread::start_thread() {
  if (!thread_started_) {
    shut_down_.store(false);
    thread_ = std::make_unique<std::thread>([this]() {
      start_thread_get_id();
      thread_function();
    });
    std::unique_lock<std::mutex> lock(lock_);
    cond_.wait(lock, [this]() { return thread_started_; });
  }
}

void processing_thread::shut_down_thread() {
  if (thread_started_) {
    shut_down_.store(true);
    cond_.notify_one();
    if (thread_->joinable())
      thread_->join();
    thread_.reset(nullptr);
    thread_started_ = false;
    id_ = {};
  }
}

void processing_thread::start_thread_get_id() {
  {
    std::lock_guard<std::mutex> lock(lock_);
    id_ = std::this_thread::get_id();
    thread_started_ = true;
  }
  cond_.notify_one();
}

bool processing_thread::is_current() const {
  return id_ == std::this_thread::get_id();
}

std::mutex& processing_thread::lock() {
  return lock_;
}

std::condition_variable& processing_thread::conditional() {
  return cond_;
}

bool processing_thread::need_shutdown() const {
  return shut_down_.load();
}

}  // namespace dolbyio::comms::transcription
