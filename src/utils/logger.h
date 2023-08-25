#pragma once

#include "interfaces/logger.h"

#include <sstream>

namespace dolbyio::comms::transcription {
class sdk_logger {
 public:
  sdk_logger(log_level level, logger& logger)
      : level_(level), sink_(logger), enabled_(sink_.is_enabled(level_)) {}
  ~sdk_logger() {
    if (enabled_) {
      auto msg = message_.str();
      sink_.log(level_, msg);
    }
  }

  template <typename T>
  sdk_logger& operator<<(const T& val) {
    if (enabled_)
      message_ << val;
    return *this;
  }

  template <typename T>
  static logger& logger_ref_from_member(T& logger) {
    if constexpr (std::is_same_v<class logger, T>) {
      return logger;
    } else {
      return *logger;
    }
  }

 private:
  log_level level_;
  logger& sink_;
  bool enabled_;
  std::stringstream message_;
};
}  // namespace dolbyio::comms::transcription

#define SDK_LOG(level)                                                     \
  ::dolbyio::comms::transcription::sdk_logger(                             \
      dolbyio::comms::log_level::level,                                    \
      ::dolbyio::comms::transcription::sdk_logger::logger_ref_from_member( \
          logger_))
