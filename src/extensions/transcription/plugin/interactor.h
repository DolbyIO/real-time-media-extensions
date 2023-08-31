/***************************************************************************
 * This program is licensed by the accompanying "license" file. This file is
 * distributed "AS IS" AND WITHOUT WARRANTY OF ANY KIND WHATSOEVER, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 *
 *                Copyright (C) 2022 - 2023 by Dolby Laboratories.
 ***************************************************************************/

#pragma once

#ifndef __linux__
#error "The live transcription addon only supports linux for now"
#endif  // !linux

#include <dolbyio/comms/sample/utilities/commands_handler_interface.h>
#include <dolbyio/comms/sample/utilities/interactor.h>
#include <dolbyio/comms/sdk.h>

#include "plugin/live_transcription.h"
#include "speech_to_text/file_writer.h"
#include "utils/signal_handler.h"

#include <string>
#include <vector>

using namespace dolbyio::comms::sample;
using namespace dolbyio::comms::rtme::transcription;


namespace dolbyio::comms::rtme {

class rtme_interactor : public dolbyio::comms::sample::interactor {
 public:
  rtme_interactor(commands_handler_interface& hdl);

  void register_command_line_handlers(
      commands_handler_interface& handler) override;
  void register_interactive_commands(
      commands_handler_interface& handler) override;

  void set_sdk(dolbyio::comms::sdk* sdk) override;

 private:
  void do_start();
  void do_stop();

  commands_handler_interface& hdl_;
  dolbyio::comms::sdk* sdk_ = nullptr;
  std::optional<
      dolbyio::comms::async_result<std::unique_ptr<live_transcription>>>
      speech_{};
  std::unique_ptr<signal_handling_helper> signal_handler_{};

  // config params:
  std::string transcript_file = {};
  std::string transcript_fifo = {};
  std::map<std::string, std::shared_ptr<transcription_service::factory>>
      transcription_service_factories_{};
  std::string transcript_service{};
  live_transcription::config engine_config_{};
};

} // namespace dolbyio::comms::rtme
