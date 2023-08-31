#include "speech_to_text/speech_to_text.h"
#include "utils/logger.h"

#include <algorithm>
#include <cstring>
#include <exception>

namespace dolbyio::comms::rtme::transcription {

char time_utc_char_array[std::size("YYYY-MM-DD hh:mm:ss.")] = {0};
static constexpr size_t seconds_in_min = 60;
static constexpr size_t seconds_in_hour = 60 * seconds_in_min;
static constexpr size_t minutes_in_hour = 60;
static constexpr size_t hours_in_day = 24;

class do_nothing_vad
    : public dolbyio::comms::audio_utils::voice_activity_detector {
 public:
  bool voice_is_active(const int16_t*, size_t, int) override { return true; }
  ~do_nothing_vad() = default;
};

void transcript_timepoint::initialize() {
  initialized_ = true;
  auto now = std::chrono::system_clock::now();
  const auto zero = std::chrono::system_clock::time_point{};
  auto sec = std::chrono::duration_cast<std::chrono::seconds>(now - zero);
  start_time_milliseconds_ =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - sec - zero)
          .count();
  start_time_seconds_ = std::chrono::system_clock::to_time_t(zero + sec);
}

transcript_timepoint::time_point transcript_timepoint::timestamp_to_utc(
    size_t timestamp_ms) {
  std::time_t seconds = (timestamp_ms / 1000) + start_time_seconds_;
  auto milliseconds = (timestamp_ms % 1000) + start_time_milliseconds_;
  if (milliseconds > 1000) {
    milliseconds %= 1000;
    ++seconds;
  }
  auto ret_tm = gmtime_r(&seconds, &tm_buffer_);
  return {ret_tm, milliseconds};
}

speech_to_text::speech_to_text(
    std::shared_ptr<transcription_listener>&& listener,
    std::shared_ptr<logger>&& logger,
    dolbyio::comms::sdk& sdk,
    const transcription_service::factory& service_factory,
    live_transcription::config&& cfg)
    : sdk_(sdk),
      logger_(std::move(logger)),
      listener_(std::move(listener)),
      transcription_wrapper_(service_factory.create(
          {this, *logger_, &sdk_, std::move(cfg.engine_params)})),
      ringbuffer_(transcription_wrapper_->get_buffer()),
      conf_tracker_(std::make_unique<conference_tracker>(sdk, ringbuffer_)),
      converter_(dolbyio::comms::audio_utils::audio_converter::create(
          g_input_channels,
          g_output_channels,
          g_input_rate,
          ringbuffer_.sample_rate())),
      vad_(cfg.use_vad ? audio_utils::voice_activity_detector::create(
                             audio_utils::voice_activity_detector::
                                 aggressiveness::very_agressive)
                       : std::make_unique<do_nothing_vad>()),
      json_("Transcript") {
  SDK_LOG(DEBUG) << "creating sppeech to text class!";
}

speech_to_text::~speech_to_text() {
  SDK_LOG(INFO) << "Destroying speech_to_text";
  conf_tracker_.reset();
  transcription_wrapper_.reset(nullptr);
  wait(sdk_.media_io().set_mixed_audio_sink(nullptr));
  SDK_LOG(INFO) << "speech_to_text destroyed";
}

// called on the webrtc thread
void speech_to_text::handle_audio(const int16_t* data,
                                  size_t n_data,
                                  int sample_rate,
                                  size_t channels) {
  process_and_store(data, sample_rate, channels, n_data);
}

async_result<void> speech_to_text::initialize() {
  conference_tracker::callbacks cbs{};
  cbs.on_start = [this](const std::string& conf_id) {
    listener_->on_transcription_started(conf_id);
    transcription_wrapper_->start();
  };
  cbs.on_exit = [this](const std::string& conf_id) {
    listener_->on_transcription_ended(conf_id);
    transcription_wrapper_->stop();
  };
  sample::async_result_accumulator accum;
  accum += sdk_.media_io().set_mixed_audio_sink(this);
  accum += conf_tracker_->initialize(cbs);
  return std::move(accum).forward();
}

void speech_to_text::process_and_store(const int16_t* data,
                                       int sample_rate,
                                       int channels,
                                       int num_data) {
  if (sample_rate != g_input_rate || channels != g_input_channels) {
    SDK_LOG(WARNING) << "Bad sample rate: " << sample_rate
                     << " or channels number: " << channels;
    return;
  }
  if (num_data != sample_rate * channels / 100) {
    // need exactly 10ms of data:
    SDK_LOG(WARNING) << "Number of input samples is not equal to 10ms: "
                     << num_data;
    return;
  }

  // Initialzie the start time of first buffer right only.
  if (!start_time_point_)
    start_time_point_.initialize();

  const size_t out_samples = ringbuffer_.sample_rate() / 100;
  auto* out_ptr = ringbuffer_.write_begin(out_samples);

  // 48000, 1ch, 10ms = 480 samples, should be enough to hold deinterleaved
  // channel data
  static constexpr int tmp_buf_size = 480;
  if (num_data > tmp_buf_size * 2) {
    SDK_LOG(WARNING) << "Temp buffer too small, this is very strange";
    return;
  }
  int16_t ch1_int[tmp_buf_size];
  int16_t ch2_int[tmp_buf_size];

  // deinterleave:
  for (int i = 0; i < num_data; i += 2) {
    ch1_int[i / 2] = data[i];
    ch2_int[i / 2] = data[i + 1];
  }

  const bool vad =
      vad_->voice_is_active(ch1_int, num_data / sizeof(int16_t), sample_rate);
  if (vad) {
    float ch1_float[tmp_buf_size];
    float ch2_float[tmp_buf_size];
    dolbyio::comms::audio_utils::audio_converter::s16_to_float(
        ch1_int, num_data / channels, ch1_float);
    dolbyio::comms::audio_utils::audio_converter::s16_to_float(
        ch2_int, num_data / channels, ch2_float);

    size_t out_size = out_samples * ringbuffer_.channels();
    float* src_channels[2] = {ch1_float, ch2_float};
    float* dst_channels[1] = {out_ptr};
    converter_->maybe_mix_andor_sample(src_channels, num_data, dst_channels,
                                       out_size);
  } else {
    memset(out_ptr, 0, ringbuffer_.sample_size() * out_samples);
  }
  ringbuffer_.write_end(out_samples, vad);
}

std::string speech_to_text::jsonisze_transcript(
    const transcription_listener::transcript& transcript,
    std::chrono::milliseconds start_latency) {
  json_.add_string("text", transcript.speech);
  json_.add_string("start_time", transcript.start_time_utc);
  json_.add_int("start_latency_ms", start_latency.count());
  json_.add_string("end_time", transcript.end_time_utc);
  json_.add_boolean("partial", transcript.partial);
  json_.add_string("id", transcript.transcript_uid);
  if (transcript.talkers.size()) {
    std::vector<json_value::key_value_set> talkers;
    talkers.reserve(transcript.talkers.size());
    for (const auto& talker : transcript.talkers) {
      json_value::key_value_set talker_set(
          {"name", talker.info.name.value_or("")}, {"user_id", talker.user_id},
          {"avatar_url", talker.info.avatar_url.value_or("")});
      talkers.push_back(std::move(talker_set));
    }
    json_.add_array("talkers", talkers);
  }
  return json_.stringify();
}

void speech_to_text::is_ready(std::string&& speech,
                              bool finalized,
                              size_t begin_timestamp,
                              size_t end_timestamp,
                              std::string unique_id) {
  if (conf_tracker_->is_conference_active() && !speech.empty()) {
    if (previous_transcript_.compare(speech, finalized)) {
      return;
    }
    auto talkers = conf_tracker_->get_talkers_at_time_point(begin_timestamp,
                                                            end_timestamp);
    std::string talkers_str{};
    for (const auto& talker : talkers) {
      if (!talkers_str.empty())
        talkers_str.append(",");
      talkers_str.append(talker.info.name.value_or("unknown name"));
    }
    SDK_LOG(DEBUG) << "New text segment: " << speech << " (talkers:["
                   << talkers_str << "]) begin: " << begin_timestamp
                   << " end: " << end_timestamp << " id: " << unique_id;
    auto transcript = transcription_listener::transcript{
        std::move(speech),
        std::move(talkers),
        unique_id,
        numsamples_to_utc_timestamp(begin_timestamp),
        numsamples_to_utc_timestamp(end_timestamp),
        !finalized};
    if (listener_->transcript_as_json()) {
      auto cur_pos = ringbuffer_.get_write_ptr();
      auto latency =
          (begin_timestamp < cur_pos) ? cur_pos - begin_timestamp : 0;
      std::chrono::milliseconds latency_ms(latency * 1000 /
                                           ringbuffer_.sample_rate());
      listener_->on_transcript(jsonisze_transcript(transcript, latency_ms),
                               finalized);
    }
    listener_->on_transcript(std::move(transcript));
  }
}

void speech_to_text::error(std::string&& description) {
  SDK_LOG(ERROR) << "Encoutered fatal error: " << description;
  listener_->transcription_status(
      transcription_listener::status::service_connection_failure);
}

std::string speech_to_text::numsamples_to_utc_timestamp(size_t samples) {
  auto sample_in_milliseconds = (samples * 1000) / ringbuffer_.sample_rate();
  auto time_point = start_time_point_.timestamp_to_utc(sample_in_milliseconds);
  std::string milliseconds_only = std::to_string(time_point.utc_millis % 1000);
  std::strftime(time_utc_char_array, sizeof(time_utc_char_array), "%F %T.",
                time_point.utc_time);
  std::string time_utc_str(time_utc_char_array,
                           sizeof(time_utc_char_array) - 1);
  time_utc_str +=
      std::string(3 - milliseconds_only.length(), '0') + milliseconds_only;
  return time_utc_str;
}

}  // namespace dolbyio::comms::rtme::transcription
