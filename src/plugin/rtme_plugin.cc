/***************************************************************************
 * This program is licensed by the accompanying "license" file. This file is
 * distributed "AS IS" AND WITHOUT WARRANTY OF ANY KIND WHATSOEVER, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 *
 *                Copyright (C) 2022 - 2023 by Dolby Laboratories.
 ***************************************************************************/

#include <dolbyio/comms/sample/utilities/commands_handler_interface.h>
#include <dolbyio/comms/sample/utilities/plugin.h>

#include "plugin/rtme_plugin_type.h"

#include <memory>

class rtme_plugin_structure : public dolbyio::comms::sample::plugin_structure {
 public:
  void start_plugin(dolbyio::comms::sample::commands_handler_interface& handler) override {
    if (interactor_)
      throw std::runtime_error("Already started");

    interactor_ = std::make_shared<dolbyio::comms::rtme::rtme_interactor>(handler);
    handler.add_interactor(interactor_);
  }

  void stop_plugin() override { interactor_ = {}; }

 private:
  std::shared_ptr<dolbyio::comms::rtme::rtme_interactor> interactor_{};
};

rtme_plugin_structure g_plugin;

extern "C" {
DOLBYIO_COMMS_RTME_EXPORT dolbyio::comms::sample::plugin_structure*
dolbyio_comms_sample_plugin_structure_get() {
  return &g_plugin;
}
}
