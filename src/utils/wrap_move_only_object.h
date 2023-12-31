#pragma once

#include <memory>

namespace dolbyio::comms::rtme {
template <typename T>
std::shared_ptr<T> wrap_move_only_object(T&& obj) {
  return std::make_shared<T>(std::move(obj));
}
}  // namespace dolbyio::comms::rtme
