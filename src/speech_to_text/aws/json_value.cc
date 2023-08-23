#include "speech_to_text/aws/json_value.h"

namespace dolbyio::comms::transcription {

json_value::json_value() : value_() {}

json_value::json_value(const std::string& str) : value_(str) {}

bool json_value::value_exists(const std::string& key) {
  return value_.View().ValueExists(key);
}

std::string json_value::get_string(const std::string& key) {
  return value_.View().GetString(key);
}

double json_value::get_double(const std::string& key) {
  return value_.View().GetDouble(key);
}

void json_value::add_boolean(const std::string& key, bool value) {
  value_.WithBool(key, value);
}

void json_value::add_int(const std::string& key, int64_t value) {
  value_.WithInt64(key, value);
}

void json_value::add_string(const std::string& key, const std::string& value) {
  value_.WithString(key, value);
}

void json_value::add_array(const std::string& key,
                           const std::vector<key_value_set>& array) {
  Aws::Utils::Array<Aws::Utils::Json::JsonValue> aws_array(array.size());
  uint8_t array_iter = 0;
  for (const auto& tuple : array) {
    Aws::Utils::Json::JsonValue value;
    value.WithString(std::get<0>(tuple).first, std::get<0>(tuple).second);
    value.WithString(std::get<1>(tuple).first, std::get<1>(tuple).second);
    value.WithString(std::get<2>(tuple).first, std::get<2>(tuple).second);
    aws_array[array_iter++] = std::move(value);
  }
  value_.WithArray(key, std::move(aws_array));
}

std::string json_value::stringify(bool compact) {
  if (compact) {
    return value_.View().WriteCompact();
  } else {
    return value_.View().WriteReadable();
  }
}

};  // namespace dolbyio::comms::transcription
