#pragma once

#include "interfaces/transcription_listener.h"

#include <memory>

#ifndef TRANSCRIPTION_EXPORT
#define TRANSCRIPTION_EXPORT __attribute__((visibility("default")))
#endif  // TRANSCRIPTION_EXPORT

namespace dolbyio::comms::transcription {

class file_writer : public transcription_listener {
 public:
  class output_formatter_policy {
   public:
    virtual ~output_formatter_policy() = default;
    virtual bool write_partials() const = 0;
    virtual void write_header(std::ofstream& file) = 0;
    virtual void write_footer(std::ofstream& file) = 0;
    virtual void write_transcript(std::ofstream& file,
                                  const std::string& transcript) = 0;
  };

  class json_formatter : public output_formatter_policy {
   public:
    bool write_partials() const override;
    void write_header(std::ofstream& file) override;
    void write_footer(std::ofstream& file) override;
    void write_transcript(std::ofstream& file,
                          const std::string& transcript) override;

   private:
    bool needs_comma_ = false;
  };

  class binary_formatter : public output_formatter_policy {
   public:
    bool write_partials() const override;
    void write_header(std::ofstream&) override;
    void write_footer(std::ofstream& file) override;
    void write_transcript(std::ofstream& file,
                          const std::string& transcript) override;
  };

  static std::shared_ptr<file_writer> create(
      std::ofstream&& file_stream,
      std::unique_ptr<output_formatter_policy> formatter);
};
}  // namespace dolbyio::comms::transcription
