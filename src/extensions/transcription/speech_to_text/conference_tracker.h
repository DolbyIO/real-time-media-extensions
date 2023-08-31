#pragma once

#include "speech_to_text/buffer/audio_ringbuffer.h"
#include "utils/async_accumulator.h"

#include <dolbyio/comms/sdk.h>

#include <deque>
#include <functional>
#include <map>
#include <mutex>
#include <vector>

namespace dolbyio::comms::rtme::transcription {
class conference_tracker {
 public:
  struct callbacks {
    std::function<void(const std::string& conf_id)> on_start{};
    std::function<void(const std::string& conf_id)> on_exit{};
  };
  conference_tracker(dolbyio::comms::sdk& sdk,
                     const audio_ringbuffer<float>& ringbuffer);
  ~conference_tracker();
  std::vector<dolbyio::comms::remote_participant> get_talkers_at_time_point(
      size_t ringbuffer_position_start,
      size_t ringbuffer_position_end);

  async_result<void> initialize(const callbacks& cbs);
  bool is_conference_active() const;

 private:
  void on_conference_status_updated(
      const dolbyio::comms::conference_status_updated& evt);
  void reset_sdk_data();
  void gc_talker_history_locked(
      size_t timestamp,
      std::chrono::seconds kill_older_than = std::chrono::seconds{10});
  dolbyio::comms::sdk& sdk_;
  const audio_ringbuffer<float>& ringbuffer_;
  std::mutex callbacks_lock_{};
  callbacks callbacks_{};
  mutable std::mutex sdk_data_lock_{};
  sample::async_result_accumulator accum_{};
  std::map<std::string, dolbyio::comms::remote_participant> participants_{};

  using talker_set = std::vector<std::string>;
  struct talker_info {
    size_t start_time;
    talker_set talkers;
  };
  std::deque<talker_info> talker_history_{};
  std::vector<dolbyio::comms::event_handler_id> event_handlers_{};
  dolbyio::comms::event_handler_id audio_levels_evt_handler_{};

  enum class active_talker_strategy {
    unknown,
    level_evts,
    speaker_evts,
  };
  active_talker_strategy ats_{active_talker_strategy::unknown};
  std::optional<std::string> local_participant_id_{};
  bool conference_is_active_ = false;
};
}  // namespace dolbyio::comms::rtme::transcription
