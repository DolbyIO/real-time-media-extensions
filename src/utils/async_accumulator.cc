/***************************************************************************
 * This program is licensed by the accompanying "license" file. This file is
 * distributed "AS IS" AND WITHOUT WARRANTY OF ANY KIND WHATSOEVER, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 *
 *                Copyright (C) 2022-2023 by Dolby Laboratories.
 ***************************************************************************/

#include "utils/async_accumulator.h"

namespace dolbyio::comms::sample {

/**
 * The core of accumlation, it contains the async_result and solver pair
 * which is returned to user and then resolved when all of the queued operations
 * have completed. It allows for increasing the number of waited for operations
 * and then decreasing the count as their solvers are resolved.
 */
class async_result_accumulator::acc {
  std::atomic<size_t> num_await = 0;
  std::exception_ptr error_{};
  async_result_with_solver<void> solver_result_pair_ =
      async_result<void>::make();
  std::mutex lock_;

 public:
  acc() = default;

  void add_waiter() {
    assert(solver_result_pair_.solver_);
    ++num_await;
  }
  void set_failure(std::exception_ptr&& err) {
    assert(solver_result_pair_.solver_);
    error_ = std::move(err);
  }
  void rem_waiter() {
    assert(solver_result_pair_.solver_);
    assert(num_await > 0);
    --num_await;
    if (num_await == 0) {
      if (error_)
        solver_result_pair_.get_solver().fail(std::move(error_));
      else
        solver_result_pair_.get_solver().resolve();
    }
  }
  async_result<void> get_result() { return solver_result_pair_.get_result(); }
};

async_result_accumulator::async_result_accumulator()
    : acc_(std::make_shared<acc>()) {
  acc_->add_waiter();
}

async_result_accumulator::~async_result_accumulator() = default;

void async_result_accumulator::add(async_result<void>&& res) {
  acc_->add_waiter();
  std::move(res)
      .then([acc{acc_}]() mutable { acc->rem_waiter(); })
      .on_error([acc{acc_}](std::exception_ptr&& err) mutable {
        acc->set_failure(std::move(err));
        acc->rem_waiter();
      });
}

async_result<void> async_result_accumulator::forward() && {
  auto res = acc_->get_result();
  acc_->rem_waiter();
  return res;
}

}  // namespace dolbyio::comms::sample
