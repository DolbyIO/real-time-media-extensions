#pragma once

#include <dolbyio/comms/logger_sink.h>

#include <memory>
#include <sstream>

namespace dolbyio::comms::rtme {
class logger {
 public:
  virtual ~logger();

  virtual bool is_enabled(log_level level) const = 0;
  virtual log_level get_level() const = 0;
  virtual void set_level(log_level level) = 0;
  virtual void log(log_level level, std::string_view message) const = 0;

  // Reimplement this method to create custom logger implementation:
  static std::shared_ptr<logger> make(
      const std::shared_ptr<logger_sink>& sdk_logger);
};

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
}  // namespace dolbyio::comms::rtme

#define SDK_LOG(level)                                                     \
  ::dolbyio::comms::rtme::sdk_logger(                             \
      dolbyio::comms::log_level::level,                                    \
      ::dolbyio::comms::rtme::sdk_logger::logger_ref_from_member( \
          logger_))
