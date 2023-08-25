#include "dolbyio/comms/media_engine/audio_utils.h"

#include "interfaces/transcription_service.h"
#include "speech_to_text/aws/json_value.h"
#include "speech_to_text/gladia/gladia.h"
#include "utils/logger.h"

#include <future>
#include <iostream>

namespace dolbyio::comms::transcription {

static constexpr char ws_url[] =
    "wss://api.gladia.io/audio/text/audio-transcription";
static constexpr char api_key[] = "x_gladia_key";
static constexpr char api_key_param_name[] = "api-key";
static constexpr char sample_rate_key[] = "sample_rate";
static constexpr char encoding_key[] = "encoding";
static constexpr char encoding_value[] = "WAV/PCM";
static constexpr char format_key[] = "frames_format";
static constexpr char format_value[] = "base64";
static constexpr size_t gladia_max_silence_send = 10;
static constexpr size_t gladia_sample_rate = 16000;
static constexpr size_t gladia_channels = 1;
static constexpr size_t gladia_sample_size_ms = 200;
static constexpr size_t gladia_ms_to_samples_factor =
    1000 / gladia_sample_size_ms;
static constexpr size_t gladia_samples_per_event =
    (gladia_sample_rate / gladia_ms_to_samples_factor);

namespace {
class gladia_factory final : public transcription_service::factory {
 public:
  std::string name() const override { return "gladia"; }
  std::map<std::string, std::string> custom_params() const override {
    std::map<std::string, std::string> settings{};
    settings.insert(
        std::make_pair(api_key_param_name, "The Gladia.io API key"));
    return settings;
  }
  std::unique_ptr<transcription_service> create(
      transcription_service::config&& config) const override {
    return std::make_unique<gladia>(std::move(config));
  }
};
}  // namespace

void websocket_listener::on_read(std::string_view data) {
  if (callback_) {
    callback_->on_data(data);
  }
}

void websocket_listener::on_closed(bool) {
  if (callback_) {
    callback_->on_status_changed(websocket_listener::callback::status::closed);
  }
}

void websocket_listener::on_reconnected() {
  if (callback_) {
    callback_->on_status_changed(
        websocket_listener::callback::status::connected);
  }
}

transcript_timestamp_tracker::transcript_timestamp_tracker(size_t latency)
    : estimate_latency_(latency),
      transcript_id_str_(std::to_string(transcript_id_)) {}

void transcript_timestamp_tracker::transcript_arrived(size_t curr_time,
                                                      size_t duration) {
  last_start_time_ = curr_time - duration - estimate_latency_;
  auto sum_avg_start_times =
      average_counter_ * avg_start_time_ + last_start_time_;
  ++average_counter_;
  avg_start_time_ = sum_avg_start_times / average_counter_;
}

size_t transcript_timestamp_tracker::calculate_end_time(size_t duration) {
  return last_start_time_ + duration;
}

void transcript_timestamp_tracker::reset_state() {
  avg_start_time_ = 0;
  average_counter_ = 0;
  ++transcript_id_;
  transcript_id_str_ = std::to_string(transcript_id_);
}

void gladia::on_data(std::string_view data) {
  SDK_LOG(VERBOSE) << "Receieved Gladia.io Data: " << data;
  json_value val{std::string(data)};
  if (!val.value_exists("transcription")) {
    return;
  }
  auto transcript = val.get_string("transcription");
  double start = 0.0, end = 0.0;
  bool partial = true;
  if (val.value_exists("type")) {
    partial = val.get_string("type") == "partial";
  }
  if (val.value_exists("time_begin")) {
    start = val.get_double("time_begin");
  }
  if (val.value_exists("time_end")) {
    end = val.get_double("time_end");
  }
  auto duration_in_samples = timestamp_to_num_samples(end - start);
  if (partial) {
    timestamp_tracker_.transcript_arrived(ringbuffer_.get_write_ptr(),
                                          duration_in_samples);
  }
  listener_->is_ready(
      std::move(transcript), partial, timestamp_tracker_.last_start_time(),
      timestamp_tracker_.calculate_end_time(duration_in_samples),
      timestamp_tracker_.transcript_id());
  if (!partial)
    timestamp_tracker_.reset_state();
}

gladia::gladia(transcription_service::config&& cfg)
    : transcription_service(cfg.listener_, cfg.logger_),
      params_(cfg.engine_params_),
      transcribe_common(gladia_sample_rate, gladia_channels, 30),
      ws_listener_(std::make_shared<websocket_listener>(this)),
      timestamp_tracker_(timestamp_to_num_samples(0.45)) {
  auto api_key_it = params_.find(api_key_param_name);
  if (api_key_it == params_.end())
    throw std::runtime_error("Gladia.io backend requires the API key!");
  SDK_LOG(INFO) << "Creating the Gladia.io websocket!";

  auto ex = dolbyio::comms::utils::netio::executor::create();
  ws_ = wait(ex->create_websocket(ws_url, ws_listener_,
                                  {std::make_pair("Connection", "Upgrade")}));
  json_value json("");
  json.add_int(sample_rate_key, gladia_sample_rate);
  json.add_string(api_key, api_key_it->second);
  json.add_string(encoding_key, encoding_value);
  json.add_string(format_key, format_value);
  if (websocket_connected()) {
    SDK_LOG(INFO) << "Sending initial Gladia.io message!";
    wait(ws_->write(std::move(json.stringify())));
  } else {
    throw std::runtime_error("Failed to create Gladia.io websocket!");
  }
}

gladia::~gladia() {
  abort_processing_.store(true);
  shut_down_thread();
  ws_.reset();
}

std::shared_ptr<transcription_service::factory> gladia::factory() {
  return std::make_shared<gladia_factory>();
}

void gladia::start() {
  abort_processing_.store(false);
  start_thread();
}

void gladia::stop() {
  abort_processing_.store(true);
  shut_down_thread();
}

void gladia::thread_function() {
  read_audio_loop(
      gladia_sample_size_ms, gladia_ms_to_samples_factor,
      [this](float* samples, size_t total_samples) {
        if (websocket_connected()) {
          ws_->write(prepare_gladia_data(samples, total_samples))
              .on_error([](auto&&) {});
          continuous_websocket_send_failures_ = 0;
        } else {
          SDK_LOG(WARNING) << "Failed to write to the websocket!";
          if (++continuous_websocket_send_failures_ > 25) {
            SDK_LOG(ERROR) << "Failed continuously to write to "
                              "websocket too many times, exiting!";
            listener_->error(
                "Transcription failed writing to socket numerously!");
          }
        }
      });
}

std::string gladia::prepare_gladia_data(float* data, size_t num_samples) {
  Aws::Utils::Array<unsigned char> buffer(gladia_samples_per_event *
                                          sizeof(int16_t));
  assert(num_samples <= gladia_samples_per_event);
  dolbyio::comms::audio_utils::audio_converter::float_to_s16(
      data, num_samples,
      reinterpret_cast<int16_t*>(buffer.GetUnderlyingData()));
  auto str = base64_.Encode(buffer);
  json_value json;
  json.add_string("frames", str);
  return json.stringify();
}

bool gladia::exit_read_audio_loop() {
  return abort_processing_;
}

}  // namespace dolbyio::comms::transcription
