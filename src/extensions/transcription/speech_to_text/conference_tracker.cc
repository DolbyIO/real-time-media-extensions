#include "speech_to_text/conference_tracker.h"

#include "utils/wrap_move_only_object.h"

#include <algorithm>
#include <set>
namespace dolbyio::comms::rtme::transcription {
conference_tracker::conference_tracker(
    sdk& sdk,
    const audio_ringbuffer<float>& ringbuffer)
    : sdk_(sdk), ringbuffer_(ringbuffer) {}

conference_tracker::~conference_tracker() {
  sample::async_result_accumulator acc{};
  acc += std::move(accum_).forward();
  for (auto& res : event_handlers_)
    acc += res->disconnect();
  if (audio_levels_evt_handler_)
    acc += audio_levels_evt_handler_->disconnect();
  audio_levels_evt_handler_.reset();
  event_handlers_.clear();
  try {
    wait(std::move(acc).forward());
  } catch (...) {
  }
}

std::vector<remote_participant> conference_tracker::get_talkers_at_time_point(
    size_t ringbuffer_position_start,
    size_t ringbuffer_position_end) {
  // find closest position:
  std::lock_guard lock(sdk_data_lock_);
  gc_talker_history_locked(ringbuffer_position_start);
  if (talker_history_.empty())
    return {};
  auto it = std::lower_bound(talker_history_.begin(), talker_history_.end(),
                             ringbuffer_position_start,
                             [](const talker_info& ti, size_t timestamp) {
                               return ti.start_time < timestamp;
                             });
  // 'it' is the talker set with timestamp larger than our timestamp. The item
  // one before it will be the one which contains current talkers. If 'it' ==
  // end(), then the last talker item contains current talkers.
  auto it_end = it;
  if (it == talker_history_.begin()) {
    if (it->start_time >= ringbuffer_position_end) {
      // if the whole range falls before the talker positions, we have no talker
      // info:
      return {};
    } else {
      // it == begin;
      // it_end - one past it, and will be checked for exact end
    }
  } else {
    it--;
    // it - begin of the range
    // it_end - one past it, will be checked for exact end
  }
  assert(it >= talker_history_.begin());
  assert(it < talker_history_.end());
  // Look for the talkers end range:
  while (it_end != talker_history_.end() &&
         it_end->start_time < ringbuffer_position_end) {
    it_end++;
  }
  // it_end is now past the range.
  std::set<std::string> known_talkers{};
  std::vector<remote_participant> ret{};
  for (; it < it_end; ++it) {
    const auto& talkers = it->talkers;
    for (const auto& talker : talkers) {
      if (known_talkers.find(talker) != known_talkers.end())
        continue;  // already have them
      known_talkers.insert(talker);
      auto part_it = participants_.find(talker);
      if (part_it == participants_.end()) {
        // unknown one??
        remote_participant unknown{};
        unknown.user_id = talker;
        ret.push_back(std::move(unknown));
      } else {
        ret.push_back(part_it->second);
      }
    }
  }
  return ret;
}

async_result<void> conference_tracker::initialize(const callbacks& cbs) {
  {
    std::lock_guard lock(callbacks_lock_);
    callbacks_ = cbs;
  }
  sample::async_result_accumulator accum{};
  accum += sdk_.conference()
               .get_current_conference()
               .then([this](conference_info&& info) {
                 conference_status_updated evt{info.status, info.id};
                 on_conference_status_updated(evt);
               })
               .consume_errors([this](auto&& e) {
                 conference_status_updated evt{conference_status::error, ""};
                 on_conference_status_updated(evt);
               });

  // attach event handlers:
  auto add_evt_handler_fn = [this]() {
    return [this](event_handler_id&& id) {
      event_handlers_.push_back(std::move(id));
    };
  };
  accum += sdk_.conference()
               .add_event_handler([this](const conference_status_updated& evt) {
                 on_conference_status_updated(evt);
               })
               .then(add_evt_handler_fn());
  accum += sdk_.conference()
               .add_event_handler([this](const remote_participant_added& pi) {
                 std::lock_guard lock(sdk_data_lock_);
                 participants_.insert_or_assign(pi.participant.user_id,
                                                pi.participant);
               })
               .then(add_evt_handler_fn());
  accum += sdk_.conference()
               .add_event_handler([this](const remote_participant_updated& pi) {
                 std::lock_guard lock(sdk_data_lock_);
                 participants_.insert_or_assign(pi.participant.user_id,
                                                pi.participant);
               })
               .then(add_evt_handler_fn());
  accum += sdk_.conference()
               .add_event_handler([this](const active_speaker_changed& evt) {
                 std::lock_guard lock(sdk_data_lock_);
                 if (ats_ == active_talker_strategy::unknown)
                   ats_ = active_talker_strategy::speaker_evts;

                 talker_info ti;
                 ti.start_time = ringbuffer_.get_write_ptr();
                 ti.talkers = evt.active_speakers;
                 talker_history_.push_back(std::move(ti));
                 gc_talker_history_locked(talker_history_.back().start_time,
                                          std::chrono::seconds{60});
               })
               .then(add_evt_handler_fn());
  accum += sdk_.conference()
               .add_event_handler([this](const audio_levels& evt) {
                 std::lock_guard lock(sdk_data_lock_);
                 switch (ats_) {
                   case active_talker_strategy::speaker_evts:
                     return;
                   case active_talker_strategy::unknown:
                     for (const auto& p : evt.levels) {
                       if (p.participant_id ==
                           "00000000-0000-0000-0000-000000000000") {
                         // This is a DVC conference, we must not use audio
                         // levels, but use active speaker change events
                         // instead:
                         ats_ = active_talker_strategy::speaker_evts;
                         return;
                       }
                     }
                     ats_ = active_talker_strategy::level_evts;
                     [[fallthrough]];
                   case active_talker_strategy::level_evts: {
                     if (!local_participant_id_)
                       break;  // we don't know who we are yet!
                     talker_info ti;
                     ti.start_time = ringbuffer_.get_write_ptr();
                     for (auto& p : evt.levels) {
                       if (p.participant_id != *local_participant_id_ &&
                           p.level > p.speaking_threshold)
                         ti.talkers.push_back(p.participant_id);
                     }
                     talker_history_.push_back(std::move(ti));
                     gc_talker_history_locked(talker_history_.back().start_time,
                                              std::chrono::seconds{60});
                   } break;
                 }
               })
               .then([this](event_handler_id&& id) {
                 std::lock_guard lock(sdk_data_lock_);
                 audio_levels_evt_handler_ = std::move(id);
               });
  auto res_slv = async_result<void>::make();
  accum_ += res_slv.get_result();

  auto slvptr = wrap_move_only_object(res_slv.get_solver());
  return std::move(accum).forward().then(
      [slvptr]() { std::move(*slvptr).resolve(); },
      [slvptr](auto&& e) { std::move(*slvptr).fail(std::move(e)); });
}

bool conference_tracker::is_conference_active() const {
  std::lock_guard lock(sdk_data_lock_);
  return conference_is_active_;
}

void conference_tracker::on_conference_status_updated(
    const conference_status_updated& evt) {
  if (evt.status == conference_status::joined) {
    {
      std::lock_guard lock(sdk_data_lock_);
      if (conference_is_active_)
        return;  // strange
      conference_is_active_ = true;
    }
    accum_ += sdk_.session().session_info().then(
        [this](services::session::user_info&& sess) {
          std::lock_guard lock(sdk_data_lock_);
          local_participant_id_ = std::move(sess.participant_id);
        });
    std::lock_guard lg(callbacks_lock_);
    if (callbacks_.on_start)
      callbacks_.on_start(evt.id);
  } else if (evt.is_ended() || evt.status == conference_status::leaving) {
    {
      {
        std::lock_guard lock(sdk_data_lock_);
        if (!conference_is_active_)
          return;
        conference_is_active_ = false;
      }
      std::lock_guard lg(callbacks_lock_);
      if (callbacks_.on_exit)
        callbacks_.on_exit(evt.id);
    }
    reset_sdk_data();
  }
}

void conference_tracker::reset_sdk_data() {
  std::lock_guard lock(sdk_data_lock_);
  talker_history_.clear();
  participants_.clear();
  local_participant_id_ = {};
  ats_ = active_talker_strategy::unknown;
}

void conference_tracker::gc_talker_history_locked(
    size_t timestamp,
    std::chrono::seconds kill_older_than) {
  size_t kill_older_than_stamp =
      ringbuffer_.sample_rate() * kill_older_than.count();

  while (talker_history_.size() > 1 &&
         talker_history_[0].start_time + kill_older_than_stamp < timestamp &&
         talker_history_[1].start_time + kill_older_than_stamp < timestamp) {
    // kill talkers older than the timestamp by more than 10 seconds, but ensure
    // that we leave at least one
    talker_history_.pop_front();
  }
}
}  // namespace dolbyio::comms::rtme::transcription
