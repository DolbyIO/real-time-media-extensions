#pragma once

#include <dolbyio/comms/logger_sink.h>

#include <memory>

namespace dolbyio::comms::transcription {
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
}  // namespace dolbyio::comms::transcription
