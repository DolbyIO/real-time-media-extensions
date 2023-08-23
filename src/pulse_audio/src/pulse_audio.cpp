#include <pulse/pulseaudio.h>

#include <string.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>

#define UNUSED(arg) (void)arg
#define VISIBLE __attribute__((visibility("default")))

using namespace std::chrono_literals;

static std::fstream logging_file;
static int log_to_file = 0;
static int log_to_output = 0;

struct pa_threaded_mainloop {
  bool running = false;
  bool lock = false;

  std::thread thread;

  pa_mainloop_api mainloop_api;
};

struct pa_io_event {
  int flags = PA_IO_EVENT_NULL;
  pa_io_event_destroy_cb_t destroy_callback;
};

struct pa_time_event {
  pa_time_event_destroy_cb_t destroy_callback;
};

struct pa_defer_event {
  int enabled;
  pa_defer_event_destroy_cb_t destroy_callback;
};

struct pa_context {
  char name[256] = {};
  pa_mainloop_api* mainloop_api = nullptr;
  pa_context_notify_cb_t callback = nullptr;
  void* userdata = nullptr;
  pa_context_state_t state = PA_CONTEXT_UNCONNECTED;
};

struct pa_operation {
  pa_operation_notify_cb_t callback = nullptr;
  void* userdata = nullptr;
  pa_context* context = nullptr;

  pa_server_info_cb_t server_info_cb_t = nullptr;

  int ref = 1;
};

struct pa_stream {
  pa_stream_state_t stream_state = PA_STREAM_UNCONNECTED;
  char name[256] = {};
  pa_sample_spec ss = {};
  pa_channel_map map = {};
};

struct pa_proplist {
};

static void mainloop(pa_threaded_mainloop* loop) {
  while (loop->running) {
    std::this_thread::sleep_for(100ms);
  }
}

#ifdef __cplusplus
extern "C" {
#endif

/// INTERNAL CONFIG

VISIBLE void fake_pulse_audio_set_log_to_file(const char* filename) {
  log_to_file = 1;
  logging_file.open(filename, std::ios::ate);
}

VISIBLE void fake_pulse_audio_set_log_to_output(int enable) {
  log_to_output = enable;
}

/// OTHER

static pa_io_event* mainloop_io_new(pa_mainloop_api* a,
                                    int fd,
                                    pa_io_event_flags_t events,
                                    pa_io_event_cb_t callback,
                                    void* userdata) {
  UNUSED(a);
  UNUSED(fd);
  UNUSED(events);
  UNUSED(callback);
  UNUSED(userdata);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  pa_io_event* e = new pa_io_event;
  return e;
}

static void mainloop_io_enable(pa_io_event* e, pa_io_event_flags_t events) {
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  e->flags = e->flags | events;
}

static void mainloop_io_free(pa_io_event* e) {
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  delete e;
}

static void mainloop_io_set_destroy(pa_io_event* e,
                                    pa_io_event_destroy_cb_t callback) {
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  e->destroy_callback = callback;
}

static pa_time_event* mainloop_time_new(pa_mainloop_api* a,
                                        const struct timeval* tv,
                                        pa_time_event_cb_t callback,
                                        void* userdata) {
  UNUSED(a);
  UNUSED(tv);
  UNUSED(callback);
  UNUSED(userdata);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  pa_time_event* e = new pa_time_event;
  return e;
}

static void mainloop_time_restart(pa_time_event* e, const struct timeval* tv) {
  UNUSED(e);
  UNUSED(tv);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
}

static void mainloop_time_free(pa_time_event* e) {
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  delete e;
}

static void mainloop_time_set_destroy(pa_time_event* e,
                                      pa_time_event_destroy_cb_t callback) {
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  e->destroy_callback = callback;
}

static pa_defer_event* mainloop_defer_new(pa_mainloop_api* a,
                                          pa_defer_event_cb_t callback,
                                          void* userdata) {
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  UNUSED(a);
  UNUSED(callback);
  UNUSED(userdata);
  pa_defer_event* e = new pa_defer_event;
  return e;
}

static void mainloop_defer_enable(pa_defer_event* e, int b) {
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  e->enabled = b;
}

static void mainloop_defer_free(pa_defer_event* e) {
  delete e;
}

static void mainloop_defer_set_destroy(pa_defer_event* e,
                                       pa_defer_event_destroy_cb_t callback) {
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  e->destroy_callback = callback;
}

static void mainloop_quit(pa_mainloop_api* a, int retval) {
  UNUSED(a);
  UNUSED(retval);

  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
}

static const pa_mainloop_api mainloop_api_vtable = {
    .userdata = NULL,

    .io_new = mainloop_io_new,
    .io_enable = mainloop_io_enable,
    .io_free = mainloop_io_free,
    .io_set_destroy = mainloop_io_set_destroy,

    .time_new = mainloop_time_new,
    .time_restart = mainloop_time_restart,
    .time_free = mainloop_time_free,
    .time_set_destroy = mainloop_time_set_destroy,

    .defer_new = mainloop_defer_new,
    .defer_enable = mainloop_defer_enable,
    .defer_free = mainloop_defer_free,
    .defer_set_destroy = mainloop_defer_set_destroy,

    .quit = mainloop_quit,
};

VISIBLE pa_stream* pa_stream_new_with_proplist(pa_context* c,
                                               const char* name,
                                               const pa_sample_spec* ss,
                                               const pa_channel_map* map,
                                               pa_proplist* proplist) {
  UNUSED(c);
  UNUSED(name);
  UNUSED(ss);
  UNUSED(map);
  UNUSED(proplist);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  return NULL;
}

VISIBLE pa_operation* pa_stream_cork(pa_stream* s,
                                     int b,
                                     pa_stream_success_cb_t cb,
                                     void* userdata) {
  UNUSED(s);
  UNUSED(b);
  UNUSED(cb);
  UNUSED(userdata);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  return NULL;
}

VISIBLE int pa_stream_is_corked(const pa_stream* s) {
  UNUSED(s);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  return 0;
}

VISIBLE int pa_stream_begin_write(pa_stream* p, void** data, size_t* nbytes) {
  UNUSED(p);
  UNUSED(data);
  UNUSED(nbytes);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  return 0;
}

VISIBLE pa_operation* pa_context_get_source_output_info(
    pa_context* c,
    uint32_t idx,
    pa_source_output_info_cb_t cb,
    void* userdata) {
  UNUSED(c);
  UNUSED(idx);
  UNUSED(cb);
  UNUSED(userdata);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  return NULL;
}

VISIBLE void pa_context_set_subscribe_callback(pa_context* c,
                                               pa_context_subscribe_cb_t cb,
                                               void* userdata) {
  UNUSED(c);
  UNUSED(cb);
  UNUSED(userdata);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
}

VISIBLE pa_operation* pa_context_subscribe(pa_context* c,
                                           pa_subscription_mask_t m,
                                           pa_context_success_cb_t cb,
                                           void* userdata) {
  UNUSED(c);
  UNUSED(m);
  UNUSED(cb);
  UNUSED(userdata);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  return NULL;
}

VISIBLE int pa_threaded_mainloop_in_thread(pa_threaded_mainloop* m) {
  UNUSED(m);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  return 0;
}

VISIBLE const char* pa_proplist_gets(const pa_proplist* p, const char* key) {
  UNUSED(p);
  UNUSED(key);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  return "";
}

VISIBLE pa_operation* pa_context_drain(pa_context* c,
                                       pa_context_notify_cb_t cb,
                                       void* userdata) {
  UNUSED(c);
  UNUSED(cb);
  UNUSED(userdata);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  return NULL;
}

VISIBLE int pa_cvolume_valid(const pa_cvolume* vol) {
  UNUSED(vol);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  return 0;
}

VISIBLE pa_operation* pa_context_set_source_output_mute(
    pa_context* c,
    uint32_t idx,
    int mute,
    pa_context_success_cb_t cb,
    void* userdata) {
  UNUSED(c);
  UNUSED(idx);
  UNUSED(mute);
  UNUSED(cb);
  UNUSED(userdata);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  return NULL;
}

VISIBLE pa_operation* pa_context_set_source_output_volume(
    pa_context* c,
    uint32_t idx,
    const pa_cvolume* vol,
    pa_context_success_cb_t cb,
    void* userdata) {
  UNUSED(c);
  UNUSED(idx);
  UNUSED(vol);
  UNUSED(cb);
  UNUSED(userdata);

  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  return NULL;
}

VISIBLE pa_operation* pa_context_set_sink_mute_by_name(
    pa_context* c,
    const char* name,
    int mute,
    pa_context_success_cb_t cb,
    void* userdata) {
  UNUSED(c);
  UNUSED(name);
  UNUSED(mute);
  UNUSED(cb);
  UNUSED(userdata);

  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  return NULL;
}

VISIBLE pa_operation* pa_context_set_source_mute_by_name(
    pa_context* c,
    const char* name,
    int mute,
    pa_context_success_cb_t cb,
    void* userdata) {
  UNUSED(c);
  UNUSED(name);
  UNUSED(mute);
  UNUSED(cb);
  UNUSED(userdata);

  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;

  return NULL;
}

VISIBLE pa_operation* pa_context_set_sink_volume_by_name(
    pa_context* c,
    const char* name,
    const pa_cvolume* vol,
    pa_context_success_cb_t cb,
    void* userdata) {
  UNUSED(c);
  UNUSED(name);
  UNUSED(vol);
  UNUSED(cb);
  UNUSED(userdata);

  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;

  return NULL;
}

VISIBLE pa_operation* pa_context_set_source_volume_by_name(
    pa_context* c,
    const char* name,
    const pa_cvolume* vol,
    pa_context_success_cb_t cb,
    void* userdata) {
  UNUSED(c);
  UNUSED(name);
  UNUSED(vol);
  UNUSED(cb);
  UNUSED(userdata);

  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;

  return NULL;
}

VISIBLE size_t pa_bytes_per_second(const pa_sample_spec* spec) {
  UNUSED(spec);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  return 0;
}

VISIBLE int pa_context_connect(pa_context* c,
                               const char* server,
                               pa_context_flags_t flags,
                               const pa_spawn_api* api) {
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  UNUSED(server);
  UNUSED(flags);
  UNUSED(api);
  c->state = PA_CONTEXT_READY;
  c->callback(c, c->userdata);
  return 0;
}

VISIBLE void pa_context_disconnect(pa_context* c) {
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  UNUSED(c);
  c->state = PA_CONTEXT_UNCONNECTED;
  c->callback(c, c->userdata);
}

VISIBLE int pa_context_errno(const pa_context* c) {
  UNUSED(c);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  return 0;
}

VISIBLE uint32_t pa_context_get_protocol_version(const pa_context* c) {
  UNUSED(c);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  return 0;
}

VISIBLE pa_operation* pa_context_get_server_info(pa_context* c,
                                                 pa_server_info_cb_t cb,
                                                 void* userdata) {
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;

  pa_server_info server_info = {};
  server_info.user_name = "User";
  server_info.host_name = "Host";
  server_info.server_version = "13.99";
  server_info.default_sink_name = "Sink";
  server_info.default_source_name = "Source";
  server_info.cookie = 1;

  server_info.sample_spec.format = PA_SAMPLE_S16LE;
  server_info.sample_spec.channels = 2;
  server_info.sample_spec.rate = 48000;
  cb(c, &server_info, userdata);

  pa_operation* o = new pa_operation;
  return o;
}

VISIBLE pa_operation* pa_context_get_sink_info_list(pa_context* c,
                                                    pa_sink_info_cb_t cb,
                                                    void* userdata) {
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;

  pa_sink_info sink_info = {};
  cb(c, &sink_info, 1, userdata);

  pa_operation* p = new pa_operation;
  return p;
}

VISIBLE pa_operation* pa_context_get_sink_info_by_index(pa_context* c,
                                                        uint32_t idx,
                                                        pa_sink_info_cb_t cb,
                                                        void* userdata) {
  UNUSED(idx);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;

  pa_sink_info sink_info = {};
  cb(c, &sink_info, 1, userdata);

  pa_operation* p = new pa_operation;
  return p;
}

VISIBLE pa_operation* pa_context_get_sink_info_by_name(pa_context* c,
                                                       const char* name,
                                                       pa_sink_info_cb_t cb,
                                                       void* userdata) {
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;

  UNUSED(name);

  pa_sink_info sink_info = {};
  cb(c, &sink_info, 1, userdata);

  pa_operation* p = new pa_operation;
  return p;
}

VISIBLE pa_operation* pa_context_get_sink_input_info(pa_context* c,
                                                     uint32_t idx,
                                                     pa_sink_input_info_cb_t cb,
                                                     void* userdata) {
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  UNUSED(idx);

  pa_sink_input_info sink_input_info = {};
  cb(c, &sink_input_info, 1, userdata);

  pa_operation* p = new pa_operation;
  return p;
}

VISIBLE pa_operation* pa_context_get_source_info_by_index(
    pa_context* c,
    uint32_t idx,
    pa_source_info_cb_t cb,
    void* userdata) {
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  UNUSED(idx);

  pa_source_info source_info = {};
  cb(c, &source_info, 1, userdata);

  pa_operation* p = new pa_operation;
  return p;
}

VISIBLE pa_operation* pa_context_get_source_info_by_name(pa_context* c,
                                                         const char* name,
                                                         pa_source_info_cb_t cb,
                                                         void* userdata) {
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;

  UNUSED(name);

  pa_source_info source_info = {};
  source_info.sample_spec.channels = 2;
  source_info.sample_spec.format = PA_SAMPLE_S16LE;
  source_info.sample_spec.rate = 48000;
  source_info.latency = 0;
  cb(c, &source_info, 1, userdata);

  pa_operation* p = new pa_operation;
  return p;
}

VISIBLE pa_operation* pa_context_get_source_info_list(pa_context* c,
                                                      pa_source_info_cb_t cb,
                                                      void* userdata) {
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;

  pa_source_info source_info = {};
  source_info.sample_spec.channels = 2;
  source_info.sample_spec.format = PA_SAMPLE_S16LE;
  source_info.sample_spec.rate = 48000;
  source_info.latency = 0;
  cb(c, &source_info, 1, userdata);

  pa_operation* p = new pa_operation;
  return p;
}

VISIBLE pa_context_state_t pa_context_get_state(const pa_context* c) {
  UNUSED(c);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  return c->state;
}

VISIBLE pa_context* pa_context_new(pa_mainloop_api* mainloop,
                                   const char* name) {
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;

  pa_context* c = new pa_context;
  strcpy(c->name, name);
  c->mainloop_api = mainloop;
  c->state = PA_CONTEXT_UNCONNECTED;
  return c;
}

VISIBLE pa_operation* pa_context_set_sink_input_volume(
    pa_context* c,
    uint32_t idx,
    const pa_cvolume* vol,
    pa_context_success_cb_t cb,
    void* userdata) {
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  UNUSED(idx);
  UNUSED(vol);

  cb(c, 1, userdata);

  pa_operation* p = new pa_operation;
  return p;
}

VISIBLE pa_operation* pa_context_set_sink_input_mute(pa_context* c,
                                                     uint32_t idx,
                                                     int mute,
                                                     pa_context_success_cb_t cb,
                                                     void* userdata) {
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  UNUSED(idx);
  UNUSED(mute);

  cb(c, 1, userdata);

  pa_operation* p = new pa_operation;
  return p;
}

VISIBLE pa_operation* pa_context_set_source_volume_by_index(
    pa_context* c,
    uint32_t idx,
    const pa_cvolume* volume,
    pa_context_success_cb_t cb,
    void* userdata) {
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;

  UNUSED(idx);
  UNUSED(volume);

  cb(c, 1, userdata);

  pa_operation* p = new pa_operation;
  return p;
}

VISIBLE pa_operation* pa_context_set_source_mute_by_index(
    pa_context* c,
    uint32_t idx,
    int mute,
    pa_context_success_cb_t cb,
    void* userdata) {
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;

  UNUSED(idx);
  UNUSED(mute);

  cb(c, 1, userdata);

  pa_operation* p = new pa_operation;
  return p;
}

VISIBLE void pa_context_set_state_callback(pa_context* c,
                                           pa_context_notify_cb_t cb,
                                           void* userdata) {
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  c->callback = cb;
  c->userdata = userdata;
}

VISIBLE void pa_context_unref(pa_context* c) {
  UNUSED(c);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
}

VISIBLE pa_cvolume* pa_cvolume_set(pa_cvolume* vol,
                                   unsigned channels,
                                   pa_volume_t vol_scalar) {
  UNUSED(vol);
  UNUSED(channels);
  UNUSED(vol_scalar);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  return NULL;
}

VISIBLE pa_operation_state_t pa_operation_get_state(const pa_operation* op) {
  UNUSED(op);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  return pa_operation_state_t::PA_OPERATION_DONE;
}

VISIBLE void pa_operation_unref(pa_operation* o) {
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  if (--(o->ref) == 0) {
    delete o;
  }
}

VISIBLE int pa_stream_connect_playback(pa_stream* s,
                                       const char* dev,
                                       const pa_buffer_attr* attr,
                                       pa_stream_flags_t flags,
                                       const pa_cvolume* volume,
                                       pa_stream* sync_stream) {
  UNUSED(s);
  UNUSED(dev);
  UNUSED(attr);
  UNUSED(flags);
  UNUSED(volume);
  UNUSED(sync_stream);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  return 0;
}

VISIBLE int pa_stream_connect_record(pa_stream* s,
                                     const char* dev,
                                     const pa_buffer_attr* attr,
                                     pa_stream_flags_t flags) {
  UNUSED(s);
  UNUSED(dev);
  UNUSED(attr);
  UNUSED(flags);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  return 0;
}

VISIBLE int pa_stream_disconnect(pa_stream* s) {
  s->stream_state = PA_STREAM_UNCONNECTED;
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  return 0;
}

VISIBLE int pa_stream_drop(pa_stream* p) {
  UNUSED(p);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  return 0;
}

VISIBLE uint32_t pa_stream_get_device_index(const pa_stream* s) {
  UNUSED(s);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  return 0;
}

VISIBLE uint32_t pa_stream_get_index(const pa_stream* s) {
  UNUSED(s);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  return 0;
}

VISIBLE int pa_stream_get_latency(pa_stream* s,
                                  pa_usec_t* r_usec,
                                  int* negative) {
  UNUSED(s);
  UNUSED(r_usec);
  UNUSED(negative);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  return 0;
}

VISIBLE const pa_sample_spec* pa_stream_get_sample_spec(pa_stream* s) {
  UNUSED(s);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  return NULL;
}

VISIBLE pa_stream_state_t pa_stream_get_state(const pa_stream* p) {
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  return p->stream_state;
}

VISIBLE pa_stream* pa_stream_new(pa_context* c,
                                 const char* name,
                                 const pa_sample_spec* ss,
                                 const pa_channel_map* map) {
  UNUSED(c);

  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  pa_stream* p = new pa_stream;
  strcpy(p->name, name);
  memcpy(&p->ss, ss, sizeof(p->ss));
  memcpy(&p->map, map, sizeof(p->map));
  return p;
}

VISIBLE int pa_stream_peek(pa_stream* p, const void** data, size_t* nbytes) {
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  UNUSED(p);
  UNUSED(data);
  UNUSED(nbytes);
  return 0;
}

VISIBLE size_t pa_stream_readable_size(const pa_stream* p) {
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  UNUSED(p);
  return 0;
}

VISIBLE pa_operation* pa_stream_set_buffer_attr(pa_stream* s,
                                                const pa_buffer_attr* attr,
                                                pa_stream_success_cb_t cb,
                                                void* userdata) {
  UNUSED(s);
  UNUSED(attr);
  UNUSED(cb);
  UNUSED(userdata);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  return NULL;
}

VISIBLE void pa_stream_set_overflow_callback(pa_stream* p,
                                             pa_stream_notify_cb_t cb,
                                             void* userdata) {
  UNUSED(p);
  UNUSED(cb);
  UNUSED(userdata);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
}

VISIBLE void pa_stream_set_read_callback(pa_stream* p,
                                         pa_stream_request_cb_t cb,
                                         void* userdata) {
  UNUSED(p);
  UNUSED(cb);
  UNUSED(userdata);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
}

VISIBLE void pa_stream_set_state_callback(pa_stream* s,
                                          pa_stream_notify_cb_t cb,
                                          void* userdata) {
  UNUSED(s);
  UNUSED(cb);
  UNUSED(userdata);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
}

VISIBLE void pa_stream_set_underflow_callback(pa_stream* p,
                                              pa_stream_notify_cb_t cb,
                                              void* userdata) {
  UNUSED(p);
  UNUSED(cb);
  UNUSED(userdata);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
}

VISIBLE void pa_stream_set_write_callback(pa_stream* p,
                                          pa_stream_request_cb_t cb,
                                          void* userdata) {
  UNUSED(p);
  UNUSED(cb);
  UNUSED(userdata);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
}

VISIBLE void pa_stream_unref(pa_stream* s) {
  UNUSED(s);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
}

VISIBLE size_t pa_stream_writable_size(const pa_stream* p) {
  UNUSED(p);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  return 0;
}

VISIBLE int pa_stream_write(pa_stream* p,
                            const void* data,
                            size_t nbytes,
                            pa_free_cb_t free_cb,
                            int64_t offset,
                            pa_seek_mode_t seek) {
  UNUSED(p);
  UNUSED(data);
  UNUSED(nbytes);
  UNUSED(free_cb);
  UNUSED(offset);
  UNUSED(seek);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  return 0;
}

VISIBLE const char* pa_strerror(int error) {
  UNUSED(error);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  return "Empty error";
}

VISIBLE void pa_threaded_mainloop_free(pa_threaded_mainloop* m) {
  delete m;
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
}

VISIBLE pa_mainloop_api* pa_threaded_mainloop_get_api(pa_threaded_mainloop* m) {
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  return (pa_mainloop_api*)(&m->mainloop_api);
}

VISIBLE void pa_threaded_mainloop_lock(pa_threaded_mainloop* m) {
  UNUSED(m);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
}

VISIBLE pa_threaded_mainloop* pa_threaded_mainloop_new(void) {
  pa_threaded_mainloop* m = new pa_threaded_mainloop;
  m->mainloop_api = mainloop_api_vtable;
  m->mainloop_api.userdata = m;
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  return m;
}

VISIBLE void pa_threaded_mainloop_signal(pa_threaded_mainloop* m,
                                         int wait_for_accept) {
  UNUSED(m);
  UNUSED(wait_for_accept);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
}

VISIBLE int pa_threaded_mainloop_start(pa_threaded_mainloop* m) {
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;

  m->running = true;
  m->thread = std::thread(mainloop, m);
  return 0;
}

VISIBLE void pa_threaded_mainloop_stop(pa_threaded_mainloop* m) {
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;

  m->running = false;
  if (m->thread.joinable()) {
    m->thread.join();
  }
}

VISIBLE void pa_threaded_mainloop_unlock(pa_threaded_mainloop* m) {
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
  m->lock = false;
}

VISIBLE void pa_threaded_mainloop_wait(pa_threaded_mainloop* m) {
  UNUSED(m);
  if (log_to_file)
    logging_file << __func__ << std::endl;
  if (log_to_output)
    std::cout << __func__ << std::endl;
}

VISIBLE int pa_proplist_sets(pa_proplist *p, const char *key, const char *value) {
  return 0;
}

VISIBLE pa_proplist* pa_proplist_new(void) {
  return new pa_proplist;
}

VISIBLE const char* pa_get_library_version(void) {
  return "0.0";
}

VISIBLE void pa_proplist_free(pa_proplist* p) {
  delete p;
}
VISIBLE pa_volume_t pa_cvolume_avg(const pa_cvolume *a) {
  return PA_VOLUME_NORM;
}

#ifdef __cplusplus
}
#endif
