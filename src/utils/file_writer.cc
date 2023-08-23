#include "utils/file_writer.h"

#include "speech_to_text/processing_thread.h"

#include <fstream>
#include <vector>

#include <fcntl.h>
#include <unistd.h>
#include <cassert>

namespace dolbyio::comms::transcription {
namespace {
class file_writer_impl : public file_writer, protected processing_thread {
 public:
  file_writer_impl(std::ofstream&& file_stream,
                   std::unique_ptr<output_formatter_policy> formatter)
      : file_(std::move(file_stream)), formatter_(std::move(formatter)) {
    if (!file_.is_open())
      throw std::runtime_error("File is not open");
    start_thread();
  }
  ~file_writer_impl() { shut_down_thread(); }

 protected:
  std::unique_ptr<output_formatter_policy> formatter_;
  std::vector<std::string> transcripts_;
  std::string last_partial_{};

 private:
  bool transcript_as_json() const override { return true; }
  void on_transcript(std::string&& json_transcript, bool final) override {
    {
      std::lock_guard l(lock());
      if (!final && !formatter_->write_partials()) {
        last_partial_ = std::move(json_transcript);
        return;
      }
      last_partial_ = {};
      transcripts_.push_back(std::move(json_transcript));
    }
    conditional().notify_one();
  }
  void on_transcript(transcript&&) override {}
  void on_transcription_started(const std::string&) override {}
  void on_transcription_ended(const std::string&) override {}
  void thread_function() override {
    std::unique_lock l(lock());
    formatter_->write_header(file_);
    std::vector<std::string> workitems{};
    while (!need_shutdown()) {
      conditional().wait(
          l, [this]() { return need_shutdown() || !transcripts_.empty(); });
      using std::swap;
      swap(workitems, transcripts_);
      l.unlock();
      for (const auto& s : workitems)
        formatter_->write_transcript(file_, s);
      workitems.clear();
      l.lock();
    }
    if (!last_partial_.empty())
      formatter_->write_transcript(file_, last_partial_);
    formatter_->write_footer(file_);
  }

  std::ofstream file_;
};

}  // namespace
std::shared_ptr<file_writer> file_writer::create(
    std::ofstream&& file_stream,
    std::unique_ptr<output_formatter_policy> formatter) {
  return std::make_shared<file_writer_impl>(std::move(file_stream),
                                            std::move(formatter));
}

bool file_writer::json_formatter::write_partials() const {
  return false;
}

void file_writer::json_formatter::write_header(std::ofstream& file) {
  const std::string begin("[");
  file.write(begin.c_str(), begin.size());
}

void file_writer::json_formatter::write_footer(std::ofstream& file) {
  const std::string end("]");
  file.write(end.c_str(), end.size());
}

void file_writer::json_formatter::write_transcript(
    std::ofstream& file,
    const std::string& transcript) {
  if (needs_comma_) {
    const std::string comma(",");
    file.write(comma.c_str(), comma.size());
  }
  file.write(transcript.c_str(), transcript.size());
  needs_comma_ = true;
}

bool file_writer::binary_formatter::write_partials() const {
  return true;
}

void file_writer::binary_formatter::write_header(std::ofstream&) {}

void file_writer::binary_formatter::write_footer(std::ofstream& file) {
  int32_t terminate = -1;
  file.write(reinterpret_cast<char*>(&terminate), sizeof(int32_t)).flush();
}

void file_writer::binary_formatter::write_transcript(
    std::ofstream& file,
    const std::string& transcript) {
  int32_t len_data = transcript.size();
  file.write(reinterpret_cast<char*>(&len_data), sizeof(int32_t))
      .write(transcript.c_str(), transcript.size())
      .flush();
}

}  // namespace dolbyio::comms::transcription
