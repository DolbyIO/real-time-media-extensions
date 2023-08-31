/***************************************************************************
 * This program is licensed by the accompanying "license" file. This file is
 * distributed "AS IS" AND WITHOUT WARRANTY OF ANY KIND WHATSOEVER, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 *
 *                Copyright (C) 2022 - 2023 by Dolby Laboratories.
 ***************************************************************************/

#include <dolbyio/comms/media_engine/audio_utils.h>
#include <dolbyio/comms/sample/utilities/commands_handler_interface.h>
#include <dolbyio/comms/sample/utilities/interactor.h>
#include <dolbyio/comms/sample/utilities/plugin.h>
#include <dolbyio/comms/sdk.h>

#include "plugin/live_transcription.h"
#include "speech_to_text/aws/aws_transcribe.h"
#include "speech_to_text/file_writer.h"
#include "speech_to_text/gladia/gladia.h"
#include "utils/signal_handler.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <vector>

#include <semaphore.h>
#include <signal.h>

using namespace dolbyio::comms::sample;
using namespace dolbyio::comms::audio_utils;

namespace dolbyio::comms::rtme {

namespace transcription {

class speech_to_text_writer : public transcription_listener {
 public:
  speech_to_text_writer(dolbyio::comms::sdk& sdk,
                        signal_handling_helper* hdl,
                        const std::shared_ptr<file_writer>& file = {})
      : sdk_(sdk), sig_hdl_(hdl), file_(file) {}

  void transcription_status(transcription_listener::status status) override {
    sig_hdl_->application_signal(
        signal_handling_helper::service_failure_exit_code);
  }

  void on_transcript(std::string&& transcript, bool final) override {
    if (file_) {
      file_->on_transcript(std::move(transcript), final);
    }
  }

  void on_transcript(transcript&& event) override {
    // Just leave it like this for now to not lose message format
    if (!transcript_as_json()) {
      std::string avatar = "";
      std::string name = "unknown participant";
      if (!event.talkers.empty()) {
        avatar = event.talkers[0].info.avatar_url.value_or("");
        name = event.talkers[0].info.name.value_or(std::string("id:") +
                                                   event.talkers[0].user_id);
      }
      std::string message =
          "{\"title\":\"Chat_Message\",\"content\":\"Transcription: "
          "{ start_time: " +
          event.start_time_utc + ", end_time: " + event.end_time_utc +
          ", transcript_id: " + event.transcript_uid +
          ", text: " + event.speech +
          ", partial: " + std::to_string(event.partial) +
          " }\",\"type\":\"text\",\"avatarUrl\":\"" + avatar +
          "\",\"name\":\"" + name + "\"}";
      sdk_.conference().send(message).on_error(
          [](auto&&) { std::cerr << "error occurred sending the message!"; });
    }
  }
  void on_transcription_started(const std::string& conference_id) override {}
  void on_transcription_ended(const std::string& conference_id) override {}

 private:
  dolbyio::comms::sdk& sdk_;
  signal_handling_helper* sig_hdl_{nullptr};
  std::shared_ptr<file_writer> file_{};
};

}  // namespace transcription

class rtme_interactor : public dolbyio::comms::sample::interactor {
 public:
  rtme_interactor(commands_handler_interface& hdl) : hdl_(hdl) {
    std::vector<std::shared_ptr<transcription::transcription_service::factory>>
        factories;
    factories.push_back(transcription::gladia::factory());
    factories.push_back(transcription::aws_wrapper::factory());
    transcript_service = factories[0]->name();
    for (const auto& f : factories) {
      transcription_service_factories_.insert(std::make_pair(f->name(), f));
    }
  }

 private:
  void register_command_line_handlers(
      commands_handler_interface& handler) override {
    assert(&handler == &hdl_);
    handler.add_command_line_switch(
        {"--rtme-transcription-vad"},
        "\n\tEnable voice activity detection to only trancribe when speaking.",
        [this]() { engine_config_.use_vad = true; });
    handler.add_command_line_switch(
        {"--rtme-transcription-logging-level"},
        "<level>\n\tLogging level for the RTME code [0..5]",
        [this](const std::string& arg) {
          auto ll = std::stoi(arg);
          if (ll < static_cast<int>(dolbyio::comms::log_level::OFF) ||
              ll > static_cast<int>(dolbyio::comms::log_level::VERBOSE))
            throw std::runtime_error(
                "RTME Logging level should be between 0 and 5");

          engine_config_.logging_level =
              static_cast<dolbyio::comms::log_level>(ll);
        });
    handler.add_command_line_switch(
        {"--rtme-transcription-file-path"},
        "<path>\n\tStore transcriptions to a file",
        [this](const std::string& arg) { transcript_file = arg; });
    handler.add_command_line_switch(
        {"--rtme-transcription-fifo-path"},
        "<path>\n\tPass transcriptions to go service using fifo",
        [this](const std::string& arg) { transcript_fifo = arg; });

    std::string trans_services{};
    for (const auto& srv : transcription_service_factories_) {
      if (!trans_services.empty())
        trans_services.append("|");
      trans_services.append(srv.first);
    }
    std::string description = "[";
    description.append(trans_services);
    description.append("]\n\tThe transcription service to use");

    handler.add_command_line_switch(
        {"--rtme-transcription-service"}, description,
        [this](const std::string& arg) {
          if (transcription_service_factories_.find(arg) ==
              transcription_service_factories_.end())
            throw std::runtime_error(
                "Bad value for --rtme-transcription-service");
          transcript_service = arg;
        });

    std::string custom_params_desc{
        "<param:value>\n\tCustom engine param. The list of params supported "
        "per engine:\n"};
    for (const auto& srv : transcription_service_factories_) {
      custom_params_desc.append("\t");
      custom_params_desc.append(srv.second->name());
      custom_params_desc.append(":\n");
      auto params = srv.second->custom_params();
      for (const auto& param : params) {
        custom_params_desc.append("\t\t");
        custom_params_desc.append(param.first);
        custom_params_desc.append(" - ");
        custom_params_desc.append(param.second);
        custom_params_desc.append("\n");
      }
    }

    handler.add_command_line_switch(
        {"--rtme-transcription-param"}, custom_params_desc,
        [this](const std::string& arg) {
          auto sep = arg.find_first_of(':');
          if (sep == arg.npos)
            throw std::runtime_error(
                "Bad argument for --rtme-transcription-param");
          auto param_name = arg.substr(0, sep);
          auto param_value = arg.substr(sep + 1);
          std::cerr << "param: " << param_name << " " << param_value
                    << std::endl;
          engine_config_.engine_params.insert(
              std::make_pair(param_name, param_value));
        });
  }

  void register_interactive_commands(
      commands_handler_interface& handler) override {
    assert(&handler == &hdl_);
  }

  void set_sdk(dolbyio::comms::sdk* sdk) override {
    if (sdk) {
      assert(!sdk_);
      sdk_ = sdk;
      do_start();
    } else {
      do_stop();
    }
  }

  void do_start() {
    assert(!speech_);
    assert(!signal_handler_);
    assert(sdk_);

    std::shared_ptr<transcription::file_writer> outfile{};
    if (!transcript_fifo.empty()) {
      std::ofstream f;
      f.open(transcript_fifo, std::ios::out | std::ios::binary);
      outfile = transcription::file_writer::create(
          std::move(f),
          std::make_unique<transcription::file_writer::binary_formatter>());
    } else if (!transcript_file.empty()) {
      std::ofstream f;
      f.open(transcript_file, std::ios::out | std::ios::binary);
      outfile = transcription::file_writer::create(
          std::move(f),
          std::make_unique<transcription::file_writer::json_formatter>());
    }

    signal_handler_ = std::make_unique<signal_handling_helper>(&hdl_);
    speech_ = transcription::live_transcription::create(
        *sdk_, transcription::live_transcription::config{engine_config_},
        transcription_service_factories_[transcript_service],
        std::make_shared<transcription::speech_to_text_writer>(
            *sdk_, signal_handler_.get(), outfile));
  }

  void do_stop() {
    if (speech_) {
      auto speech = wait(std::move(speech_).value());
      speech_ = {};
    }
    signal_handler_.reset();
    sdk_ = nullptr;
  }

  commands_handler_interface& hdl_;
  dolbyio::comms::sdk* sdk_ = nullptr;
  std::optional<dolbyio::comms::async_result<
      std::unique_ptr<transcription::live_transcription>>>
      speech_{};
  std::unique_ptr<signal_handling_helper> signal_handler_{};

  // config params:
  std::string transcript_file = {};
  std::string transcript_fifo = {};
  std::map<std::string,
           std::shared_ptr<transcription::transcription_service::factory>>
      transcription_service_factories_{};
  std::string transcript_service{};
  transcription::live_transcription::config engine_config_{};
};

}  // namespace dolbyio::comms::rtme
