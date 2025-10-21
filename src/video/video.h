#pragma once

#include <video_decoder.h>
#include <algorithm>
#include <chrono>
#include <queue>

using VideoCallback = std::function<void(const std::unique_ptr<uint8_t[]> &pixels)>;

class Video {
 public:
  Video(std::string path, int32_t video_width, int32_t video_height)
      : width(video_width), height(video_height), _decoder(path, video_width, video_height) {
    consume_clock_time();

    _pixel_data = std::make_unique<uint8_t[]>(width * height * 4);
  }

  bool start() {
    return _decoder.start();
  }

  void play() {
    _is_playing = true;
  }

  void pause() {
    _is_playing = false;
  }

  void seek(double target) {
    auto backwards_seek = false;

    if (playback_position > target) {
      backwards_seek = true;
    }

    playback_position = target;

    if (backwards_seek) {
      _decoder.seek(target);
      _available_frames = {};
    }
  }

  double duration() {
    return _decoder.duration;
  }

  void set_loop(bool value) {
    _decoder.looping = value;
  }

  bool get_loop() {
    return _decoder.looping;
  }

  void update(VideoCallback on_tex_update) {
    update_time();

    if (_decoder.state == DecoderState::EndOfStream && _available_frames.size() == 0) {
      if (playback_position < _decoder.last_decoded_frame_time) {
        seek_into_sync();
      }
    }

    while (!_available_frames.empty() && is_next_frame_valid(_available_frames.front())) {
      _last_frame = _available_frames.front();

      _available_frames.pop();

      _last_frame_shown = false;
    }

    std::optional<DecodedFrame> peek_frame = std::nullopt;

    if (_available_frames.size() > 0) {
      peek_frame = _available_frames.front();
    }

    auto out_of_sync = false;

    if (peek_frame.has_value()) {
      out_of_sync = std::abs(playback_position - peek_frame->time) > lenience_before_seek;

      if (get_loop()) {
        out_of_sync &= std::abs(playback_position - _decoder.duration - peek_frame->time) > lenience_before_seek &&
                       std::abs(playback_position + _decoder.duration - peek_frame->time) > lenience_before_seek;
      }
    }

    if (out_of_sync) {
      _decoder.seek(playback_position);
      _available_frames = {};
    }

    if (!_last_frame_shown && _last_frame.has_value()) {
      const auto pixels = _pixel_data.get();

      const int32_t rgba_stride[4] = {4 * width, 0, 0, 0};
      sws_scale(_decoder.sws_ctx, _last_frame->frame->data, _last_frame->frame->linesize, 0, _last_frame->frame->height, &pixels, rgba_stride);

      on_tex_update(_pixel_data);

      _last_frame_shown = true;
    }

    if (_available_frames.empty()) {
      for (const auto &frame : _decoder.get_decoded_frames()) {
        _available_frames.push(frame);
      }
    }
  }

  bool is_playing() {
    return _is_playing;
  }

  double playback_position = 0;

  int32_t width;
  int32_t height;

  double speed = 1.0;

 private:
  void seek_into_sync() {
    _decoder.seek(playback_position);

    while (!_available_frames.empty()) {
      _available_frames.pop();
    }
  }

  bool is_next_frame_valid(DecodedFrame frame) {
    if (get_loop() && std::abs((frame.time - duration()) - playback_position) < lenience_before_seek) {
      return true;
    }

    return frame.time <= playback_position && std::abs(frame.time - playback_position) < lenience_before_seek;
  }

  void update_time() {
    const auto consumed_time = consume_clock_time();

    if (_is_playing) {
      playback_position += consumed_time * speed;

      if (get_loop()) {
        playback_position = std::fmod(playback_position, duration());
      }

      playback_position = std::clamp(static_cast<double>(playback_position), 0.0, _decoder.duration);
    }
  }

  double consume_clock_time() {
    const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    const auto elapsed = now.count() - _last_time;
    _last_time = now.count();

    return elapsed;
  }

  const float lenience_before_seek = 2500;

  bool _is_playing = false;

  double _last_time = 0;

  std::queue<DecodedFrame> _available_frames;

  bool _last_frame_shown = false;
  std::optional<DecodedFrame> _last_frame;

  VideoDecoder _decoder;

  std::unique_ptr<uint8_t[]> _pixel_data;
};
