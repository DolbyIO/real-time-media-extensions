#pragma once

/***************************************************************************
 * This program is licensed by the accompanying "license" file. This file is
 * distributed "AS IS" AND WITHOUT WARRANTY OF ANY KIND WHATSOEVER, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 *
 *                Copyright (C) 2022-2023 by Dolby Laboratories.
 ***************************************************************************/

#include <dolbyio/comms/async_result.h>

namespace dolbyio::comms::sample {

/**
 * A helper class which allows queueing of operations that can be
 * executed without waiting for one another and then resolves an
 * async_result when all the operations have completed.
 */
class async_result_accumulator {
 public:
  async_result_accumulator();
  ~async_result_accumulator();

  void add(async_result<void>&& res);
  async_result<void> forward() &&;

  operator async_result<void>() && { return std::move(*this).forward(); }

  async_result_accumulator& operator+=(async_result<void>&& res) {
    add(std::move(res));
    return *this;
  }

 private:
  class acc;
  std::shared_ptr<acc> acc_;
};

}  // namespace dolbyio::comms::sample
