#include "utils/logger.h"

namespace dolbyio::comms::rtme {

logger::~logger() = default;

namespace {
class logger_impl : public logger {
 public:
  logger_impl(const std::shared_ptr<logger_sink>& sdk_logger)
      : sdk_logger_(sdk_logger) {}
  void log(log_level level, std::string_view message) const override {
    sdk_logger_->log(level, message);
  }
  bool is_enabled(log_level level) const override {
    return sdk_logger_->is_enabled(level);
  }
  log_level get_level() const override { return sdk_logger_->get_level(); }
  void set_level(log_level level) override { sdk_logger_->set_level(level); }

 private:
  std::shared_ptr<logger_sink> sdk_logger_;
};
}  // namespace

std::shared_ptr<logger> logger::make(
    const std::shared_ptr<logger_sink>& sdk_logger) {
  return std::make_shared<logger_impl>(sdk_logger);
}

}  // namespace dolbyio::comms::transcription
