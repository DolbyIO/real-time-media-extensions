#pragma once

#include <dolbyio/comms/participant_info.h>

#include <string>
#include <vector>

#ifndef TRANSCRIPTION_EXPORT
#define TRANSCRIPTION_EXPORT __attribute__((visibility("default")))
#endif  // TRANSCRIPTION_EXPORT

namespace dolbyio::comms::rtme::transcription {
class TRANSCRIPTION_EXPORT transcription_listener {
 public:
  enum class status {
    service_connection_failure,
    idle_timeout_failure,
  };

  struct TRANSCRIPTION_EXPORT transcript {
    std::string speech;
    std::vector<dolbyio::comms::remote_participant> talkers;
    std::string transcript_uid;
    std::string start_time_utc;
    std::string end_time_utc;
    bool partial;
  };
  virtual ~transcription_listener();

  virtual bool transcript_as_json() const { return true; };
  virtual void on_transcript(std::string&& json_transcript, bool final) = 0;
  virtual void on_transcript(transcript&& event) = 0;

  virtual void on_transcription_started(const std::string& conference_id) = 0;
  virtual void on_transcription_ended(const std::string& conference_id) = 0;
  virtual void transcription_status(status) {}
};
}  // namespace dolbyio::comms::rtme::transcription
