#include "utils/signal_handler.h"

#include <cassert>
#include <iostream>

namespace dolbyio::comms::transcription {

signal_handling_helper::signal_handling_helper(
    sample::commands_handler_interface* hdl)
    : hdl(hdl) {
  sem_init(&sem, 0, 0);

  if (g_instance != nullptr)
    throw std::runtime_error(
        "Signal handling is global, this class is a sigleton");

  g_instance = this;

  signal(SIGTERM, [](int signum) {
    assert(g_instance);
    g_instance->on_signal(signum, sigterm_exit_code);
  });
  signal(SIGINT, [](int signum) {
    assert(g_instance);
    g_instance->on_signal(signum, sigint_exit_code);
  });
  t = std::make_unique<std::thread>([this]() { thread_function(); });
}

signal_handling_helper::~signal_handling_helper() {
  if (t->joinable()) {
    exit_thread = true;
    sem_post(&sem);
    t->join();
  }

  signal(SIGINT, SIG_DFL);
  signal(SIGTERM, SIG_DFL);

  if (g_instance == this)
    g_instance = nullptr;

  sem_destroy(&sem);
}

void signal_handling_helper::application_signal(int exit_code) {
  {
    if (internal_exit_code >= 0) {
      return;
    }
    internal_exit_code = exit_code;
  }
  sem_post(&sem);
}

void signal_handling_helper::on_signal(int signum, int exit_code) {
  if (signum != SIGTERM && signum != SIGINT) {
    std::cerr << "Unprepared signal received!\n";
  }
  external_exit_code = exit_code;
  sem_post(&sem);
}

void signal_handling_helper::start_watchdog_thread() {
  std::thread wdg([ec{external_exit_code}]() {
    std::this_thread::sleep_for(std::chrono::seconds(10));
    _Exit(ec);
  });
  wdg.detach();
}

void signal_handling_helper::thread_function() {
  while (true) {
    sem_wait(&sem);
    if (exit_thread) {
      return;
    }
    int exit_code = internal_exit_code;
    if (exit_code < 0) {
      exit_code = external_exit_code;
    }
    if (exit_code >= 0) {
      start_watchdog_thread();
      hdl->quit(exit_code);
      return;
    }
  }
}

}  // namespace dolbyio::comms::transcription
