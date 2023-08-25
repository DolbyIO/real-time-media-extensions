#pragma once

#include <dolbyio/comms/sample/utilities/commands_handler_interface.h>

#include <atomic>
#include <thread>
#include <semaphore.h>
#include <signal.h>

namespace dolbyio::comms::transcription {

class signal_handling_helper {
  inline static signal_handling_helper* g_instance = nullptr;
 public:
  static constexpr int sigterm_exit_code = 5;
  static constexpr int sigint_exit_code = 6;
  static constexpr int service_failure_exit_code = 7;

  signal_handling_helper(sample::commands_handler_interface* hdl);
  ~signal_handling_helper();

  void application_signal(int exit_code);

 private:
  void on_signal(int signum, int exit_code);
  void start_watchdog_thread();
  void thread_function();

  sample::commands_handler_interface* hdl = nullptr;
  volatile sig_atomic_t external_exit_code = -1;
  std::atomic<int> internal_exit_code = -1;  // -1 no exit
  std::atomic<bool> exit_thread = false;
  sem_t sem{};
  std::unique_ptr<std::thread> t{};
};

}
