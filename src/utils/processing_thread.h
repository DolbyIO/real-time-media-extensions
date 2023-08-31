#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace dolbyio::comms::rtme::transcription {

class semaphore {
 public:
  explicit semaphore(int count, size_t max_count);
  ~semaphore();

  void post();
  void wait();
  bool wait_timeout(std::chrono::milliseconds&& timeout);
 private:
  int count_{0};
  size_t max_count_{0};
  std::mutex lock_;
  std::condition_variable cond_;
};

class processing_thread {
 public:
  processing_thread();
  ~processing_thread();

 protected:
  virtual void thread_function() = 0;
  void shut_down_thread();
  void start_thread();

  bool is_current() const;
  std::mutex& lock();
  std::condition_variable& conditional();
  bool need_shutdown() const;
  std::atomic<bool> abort_processing_{false};

 private:
  void start_thread_get_id();

  bool thread_started_{false};
  std::atomic<bool> shut_down_{false};
  std::mutex lock_;
  std::condition_variable cond_;
  std::unique_ptr<std::thread> thread_;
  std::thread::id id_;
};

}  // namespace dolbyio::comms::rtme::transcription
