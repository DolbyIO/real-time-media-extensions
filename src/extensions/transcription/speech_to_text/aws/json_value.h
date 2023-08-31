#pragma once

#include <aws/core/utils/json/JsonSerializer.h>

#include <string>
#include <vector>

namespace dolbyio::comms::rtme::transcription {

class json_value {
 public:
  using key_value = std::pair<std::string, std::string>;
  using key_value_set = std::tuple<key_value, key_value, key_value>;

  json_value();
  json_value(const std::string& json);

  bool value_exists(const std::string& key);
  std::string get_string(const std::string& key);
  double get_double(const std::string& key);

  void add_string(const std::string& key, const std::string& value);
  void add_boolean(const std::string& key, bool value);
  void add_int(const std::string& key, int64_t value);
  void add_array(
      const std::string& key,
      const std::vector<key_value_set>& array);
  std::string stringify(bool compact = true);

 private:
  Aws::Utils::Json::JsonValue value_;
};

}  // namespace dolbyio::comms::rtme::transcription
