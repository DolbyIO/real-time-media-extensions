#include "dolbyio/comms/media_engine/audio_utils.h"

#include "speech_to_text/aws/aws_transcribe.h"
#include "utils/logger.h"

namespace dolbyio::comms::rtme::transcription {

static constexpr size_t aws_transcribe_max_silence_send = 10;
static constexpr size_t aws_transcribe_sample_rate = 16000;
static constexpr size_t aws_transcribe_channels = 1;
static constexpr size_t aws_transcribe_sample_size_ms = 50;
static constexpr size_t aws_transcribe_ms_to_samples_factor =
    1000 / aws_transcribe_sample_size_ms;
static constexpr size_t aws_transcribe_samples_per_event =
    (aws_transcribe_sample_rate / aws_transcribe_ms_to_samples_factor);

namespace {
class aws_factory final : public transcription_service::factory {
 public:
  std::string name() const override { return "aws"; }
  std::map<std::string, std::string> custom_params() const override {
    return {};
  }
  std::unique_ptr<transcription_service> create(
      transcription_service::config&& config) const override {
    Aws::SDKOptions options;
    auto ll = static_cast<int>(config.logger_.get_level());
    options.loggingOptions.logLevel =
        static_cast<Aws::Utils::Logging::LogLevel>(ll);
    Aws::InitAPI(options);
    return std::make_unique<aws_wrapper>(config.listener_, config.logger_,
                                         std::move(options));
  }
};
}  // namespace

std::shared_ptr<transcription_service::factory> aws_wrapper::factory() {
  return std::make_shared<aws_factory>();
}

aws_wrapper::aws_wrapper(transcription_service::text_listener* listener,
                         logger& logger,
                         Aws::SDKOptions&& options)
    : transcription_service(listener, logger),
      transcribe_common(aws_transcribe_sample_rate,
                        aws_transcribe_channels,
                        30),
      aws_options_(std::move(options)) {
  SDK_LOG(INFO) << "AWS wrapper constructor!";
}

aws_wrapper::~aws_wrapper() {
  abort_processing_.store(true);
  aws_transcribe_client_.DisableRequestProcessing();
  shut_down_thread();
  Aws::ShutdownAPI(aws_options_);
  SDK_LOG(INFO) << "Leaving AWS wrapper destructor, API is shutdwon!";
}

void aws_wrapper::start() {
  abort_processing_.store(false);
  aws_transcribe_client_.EnableRequestProcessing();
  start_thread();
}

void aws_wrapper::stop() {
  abort_processing_.store(true);
  aws_transcribe_client_.DisableRequestProcessing();
  shut_down_thread();
  SDK_LOG(INFO) << "Stopped the AWS processing thread!";
}

void aws_wrapper::thread_function() {
  // Callback when the http request completes, AWSSDK executor thread. There is
  // somethign inherintely wrong with request handling. It appears to only be
  // invoked after you exit the stream_ready_handler, and sometimes is not
  // invoked at all. I found various AWSSDK issues related to it but nothing
  // that seems like it solves it so what we do is wait for max 3 seconds and if
  // no error then shutterdown. So this callback doesn't actually tell you if
  // the inital request succeeded or not.
  semaphore semaphore(0, 1);
  auto request_handler =
      [this, &semaphore](
          const Aws::TranscribeStreamingService::
              TranscribeStreamingServiceClient*,
          const Aws::TranscribeStreamingService::Model::
              StartStreamTranscriptionRequest&,
          const Aws::TranscribeStreamingService::Model::
              StartStreamTranscriptionOutcome& outcome,
          const std::shared_ptr<const Aws::Client::AsyncCallerContext>&) {
        bool abort_requested =
            abort_processing_.load(std::memory_order_acquire);
        abort_processing_.store(true);
        if (!abort_requested && !outcome.IsSuccess()) {
          SDK_LOG(ERROR) << "Request to start transcribe failed: "
                         << outcome.GetError();
          listener_->error("Transcription failed to start");
        } else {
          SDK_LOG(INFO) << "Request to start transcribe has succeeded!";
        }
        semaphore.post();
      };
  // Callback once the Aws::AudioStream is ready, meaning that previous http
  // request was signed. This happens before the request handler and all passing
  // of audio to AWS runs from here. This is invoked on this thread.
  auto stream_ready_handler =
      [this](Aws::TranscribeStreamingService::Model::AudioStream& stream) {
        SDK_LOG(INFO) << "Aws::AudioStream is ready start processing!";
        run_aws_audio_transcription(stream);
        stream.flush();
        stream.Close();
        SDK_LOG(INFO)
            << "Transcription finished Aws::AudioStream flushed/closed!";
      };
  // The transcription callback invoked by AWS SDK when receiving the
  // transcripts.
  aws_transcribe_handler_.SetTranscriptEventCallback(
      [this](const Aws::TranscribeStreamingService::Model::TranscriptEvent&
                 event) mutable { handle_incoming_transcript(event); });

  // Creating the the transcription start request. We have set partial
  // stabilization to true for faster results better for realtime transcription.
  Aws::TranscribeStreamingService::Model::StartStreamTranscriptionRequest
      request;
  request.SetMediaEncoding(
      Aws::TranscribeStreamingService::Model::MediaEncoding::pcm);
  request.SetMediaSampleRateHertz(16000);
  request.SetLanguageCode(
      Aws::TranscribeStreamingService::Model::LanguageCode::en_US);
  request.SetEventStreamHandler(aws_transcribe_handler_);
  request.SetEnablePartialResultsStabilization(true);
  request.SetPartialResultsStability(
      Aws::TranscribeStreamingService::Model::PartialResultsStability::high);
  SDK_LOG(INFO) << "Callig Aws StartStreamTranscriptionAsync!";
  aws_transcribe_client_.StartStreamTranscriptionAsync(
      request, stream_ready_handler, request_handler, nullptr);

  // The assumption was that the request_handler would be invoked relatively
  // soon after this request is made. But it appears that it is invoked only
  // after stopping the aws streaming. Sometimes its invoked right away after,
  // sometimes it not invoked at all and just timesout. Probably something is
  // being done wrong but have not been able to find what it is. So we wait for
  // a bit after closing the stream, then if the cond times out we cancel the
  // outstanding requests and wait for the failure. After which we shutdown the
  // client.
  if (!semaphore.wait_timeout(std::chrono::seconds(1))) {
    aws_transcribe_client_.DisableRequestProcessing();
    semaphore.wait();
  }
  Aws::TranscribeStreamingService::TranscribeStreamingServiceClient::
      ShutdownSdkClient(&aws_transcribe_client_, 1000);
  SDK_LOG(INFO) << "AWS Transcription Client shutdown Leaving the thread "
                   "processing function!";
}

void aws_wrapper::run_aws_audio_transcription(
    Aws::TranscribeStreamingService::Model::AudioStream& audio_stream) {
  assert(ringbuffer_.sample_rate() == aws_transcribe_sample_rate);
  assert(ringbuffer_.channels() == aws_transcribe_channels);
  read_audio_loop(
      aws_transcribe_sample_size_ms, aws_transcribe_ms_to_samples_factor,
      [this, &audio_stream](float* samples, size_t total_samples) {
        audio_stream.WriteAudioEvent(prepare_aws_data(samples, total_samples));
      });
}

void aws_wrapper::handle_incoming_transcript(
    const Aws::TranscribeStreamingService::Model::TranscriptEvent& event) {
  if (!abort_processing_) {
    bool finalized = false;
    // For now this is ok since we are using a single channel. Later we will
    // will need to check which result is for what.
    for (auto&& r : event.GetTranscript().GetResults()) {
      if (r.GetIsPartial()) {
        SDK_LOG(DEBUG) << "[Partial] start: " << r.GetStartTime()
                       << "s end: " << r.GetEndTime() << "s";
      } else {
        SDK_LOG(INFO) << "[Final] start: " << r.GetStartTime()
                      << "s end: " << r.GetEndTime() << "s";
        finalized = true;
      }
      if (listener_) {
        for (auto&& alt : r.GetAlternatives()) {
          auto val = alt.GetTranscript();
          SDK_LOG(DEBUG) << val;
          auto start = timestamp_to_num_samples(r.GetStartTime());
          auto end = timestamp_to_num_samples(r.GetEndTime());
          listener_->is_ready(std::move(val), finalized, start, end,
                              r.GetResultId());
        }
      }
    }
    if (transcript_finalized_ != finalized) {
      transcript_finalized_.store(finalized);
    }
  }
}

Aws::TranscribeStreamingService::Model::AudioEvent
aws_wrapper::prepare_aws_data(float* data, size_t num_samples) {
  Aws::Vector<unsigned char> buffer(num_samples * sizeof(int16_t));
  assert(num_samples <= aws_transcribe_samples_per_event);
  dolbyio::comms::audio_utils::audio_converter::float_to_s16(
      data, num_samples, reinterpret_cast<int16_t*>(buffer.data()));
  return {std::move(buffer)};
}

bool aws_wrapper::exit_read_audio_loop() {
  return abort_processing_;
}

}  // namespace dolbyio::comms::rtme::transcription
