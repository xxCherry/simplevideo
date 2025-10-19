#include <napi.h>
#include <raylib.h>
#include <rlgl.h>
#include <video.h>

#define ARG_CHECK(arg_count)                                                         \
  if (args.Length() != arg_count) {                                                  \
    Napi::Error::New(env, "Wrong number of arguments").ThrowAsJavaScriptException(); \
    return env.Null();                                                               \
  }

namespace {

using ExportT = Napi::Value(const Napi::CallbackInfo &args);

Napi::Value video_create(const Napi::CallbackInfo &args) {
  const auto env = args.Env();

  ARG_CHECK(3);

  const auto path = args[0].As<Napi::String>();

  const auto width = args[1].As<Napi::Number>().Uint32Value();
  const auto height = args[2].As<Napi::Number>().Uint32Value();

  auto video = new Video(path.Utf8Value(), width, height);

  return Napi::External<Video>::New(env, video);
}

Napi::Value video_start(const Napi::CallbackInfo &args) {
  const auto env = args.Env();

  ARG_CHECK(1);

  const auto video = args[0].As<Napi::External<Video>>();

  video.Data()->start();

  return env.Undefined();
}

Napi::Value video_play(const Napi::CallbackInfo &args) {
  const auto env = args.Env();

  ARG_CHECK(1);

  const auto video = args[0].As<Napi::External<Video>>();

  video.Data()->play();

  return env.Undefined();
}

Napi::Value video_pause(const Napi::CallbackInfo &args) {
  const auto env = args.Env();

  ARG_CHECK(1);

  const auto video = args[0].As<Napi::External<Video>>();

  video.Data()->pause();

  return env.Undefined();
}

Napi::Value video_get_time(const Napi::CallbackInfo &args) {
  const auto env = args.Env();

  ARG_CHECK(1);

  const auto video = args[0].As<Napi::External<Video>>();

  return Napi::Number::From(env, video.Data()->playback_position);
}

Napi::Value video_get_duration(const Napi::CallbackInfo &args) {
  const auto env = args.Env();

  ARG_CHECK(1);

  const auto video = args[0].As<Napi::External<Video>>();

  return Napi::Number::From(env, video.Data()->duration());
}

Napi::Value video_get_speed(const Napi::CallbackInfo &args) {
  const auto env = args.Env();

  ARG_CHECK(1);

  const auto video = args[0].As<Napi::External<Video>>();

  return Napi::Number::From(env, video.Data()->speed);
}

Napi::Value video_set_speed(const Napi::CallbackInfo &args) {
  const auto env = args.Env();

  ARG_CHECK(2);

  const auto video = args[0].As<Napi::External<Video>>();
  const auto speed = args[1].As<Napi::Number>().DoubleValue();

  video.Data()->speed = speed;

  return env.Undefined();
}

Napi::Value video_seek(const Napi::CallbackInfo &args) {
  const auto env = args.Env();

  ARG_CHECK(2);

  const auto video = args[0].As<Napi::External<Video>>();
  const auto target = args[1].As<Napi::Number>().DoubleValue();

  video.Data()->seek(target);

  return env.Undefined();
}

Napi::Value video_get_loop(const Napi::CallbackInfo &args) {
  const auto env = args.Env();

  ARG_CHECK(1);

  const auto video = args[0].As<Napi::External<Video>>();

  return Napi::Boolean::From(env, video.Data()->get_loop());
}

Napi::Value video_set_loop(const Napi::CallbackInfo &args) {
  const auto env = args.Env();

  ARG_CHECK(2);

  const auto video = args[0].As<Napi::External<Video>>();
  const auto loop = args[1].As<Napi::Boolean>();

  video.Data()->set_loop(loop.Value());

  return env.Undefined();
}

Napi::Value video_is_playing(const Napi::CallbackInfo &args) {
  const auto env = args.Env();

  ARG_CHECK(1);

  const auto video = args[0].As<Napi::External<Video>>();

  return Napi::Boolean::From(env, video.Data()->is_playing());
}

Napi::Value video_update(const Napi::CallbackInfo &args) {
  const auto env = args.Env();

  ARG_CHECK(2);

  const auto video = args[0].As<Napi::External<Video>>();
  const auto on_texture_update = args[1].As<Napi::Function>();

  video.Data()->update([&](const std::unique_ptr<uint8_t[]> &pixels) {
    on_texture_update.Call({Napi::Number::From(env, reinterpret_cast<uintptr_t>(pixels.get()))});
  });

  return env.Undefined();
}

Napi::Value video_destroy(const Napi::CallbackInfo &args) {
  const auto env = args.Env();

  ARG_CHECK(1);

  const auto video = args[0].As<Napi::External<Video>>();

  delete video.Data();

  return env.Undefined();
}

}  // namespace

Napi::Object register_exports(Napi::Env env, Napi::Object exports) {
  const auto register_export = [&](std::string name, ExportT export_fn) {
    exports.Set(Napi::String::New(env, name), Napi::Function::New(env, export_fn));
  };

  register_export("videoCreate", video_create);
  register_export("videoStart", video_start);
  register_export("videoPlay", video_play);
  register_export("videoPause", video_pause);
  register_export("videoUpdate", video_update);
  register_export("videoDestroy", video_destroy);
  register_export("videoGetTime", video_get_time);
  register_export("videoGetDuration", video_get_duration);
  register_export("videoGetSpeed", video_get_speed);
  register_export("videoSetSpeed", video_set_speed);
  register_export("videoGetLoop", video_get_loop);
  register_export("videoSetLoop", video_set_loop);
  register_export("videoSeek", video_seek);
  register_export("videoIsPlaying", video_is_playing);

  return exports;
}

NODE_API_MODULE(nfd, register_exports);
