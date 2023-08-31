#pragma once

#include <aws/core/Aws.h>
#include <aws/transcribestreaming/TranscribeStreamingServiceClient.h>
#include <aws/transcribestreaming/model/StartStreamTranscriptionHandler.h>
#include <aws/transcribestreaming/model/StartStreamTranscriptionRequest.h>

#include "interfaces/transcription_service.h"
#include "utils/processing_thread.h"
#include "speech_to_text/transcribe_common.h"

namespace dolbyio::comms::rtme::transcription {

class aws_wrapper : public transcription_service,
                    public processing_thread,
                    public transcribe_common {
 public:
  aws_wrapper(transcription_service::text_listener* listener,
              logger& logger,
              Aws::SDKOptions&& options);
  ~aws_wrapper() override;
  static std::shared_ptr<factory> factory();
  static std::unique_ptr<transcription_service> create(
      transcription_service::text_listener* listener,
      logger& logger);
  audio_ringbuffer<float>& get_buffer() override { return ringbuffer_; }
  void thread_function() override;
  void start() override;
  void stop() override;

 private:
  bool exit_read_audio_loop() override;
  void run_aws_audio_transcription(
      Aws::TranscribeStreamingService::Model::AudioStream& audio_stream);
  void handle_incoming_transcript(
      const Aws::TranscribeStreamingService::Model::TranscriptEvent& event);
  Aws::TranscribeStreamingService::Model::AudioEvent prepare_aws_data(
      float* data,
      size_t num_samples);

  Aws::SDKOptions aws_options_{};
  Aws::TranscribeStreamingService::TranscribeStreamingServiceClient
      aws_transcribe_client_;
  Aws::TranscribeStreamingService::Model::StartStreamTranscriptionHandler
      aws_transcribe_handler_{};
};

}  // namespace dolbyio::comms::rtme::transcription
