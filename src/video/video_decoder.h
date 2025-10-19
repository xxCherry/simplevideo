#pragma once

#include <functional>
#include <optional>
#include <print>
#include <thread>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include <concurrent_queue.h>
#include <chrono>
#include <string>

using namespace std::chrono_literals;

enum class DecoderState { Ready, Running, EndOfStream };

struct DecodedFrame {
  double time;
  AVFrame *frame;
};

class VideoDecoder {
 public:
  VideoDecoder(std::string path, int32_t width, int32_t height) : _path(path), _width(width), _height(height) {
    state = DecoderState::Ready;
  }

  bool start() {
    if (!prepare()) {
      return false;
    }

    if (!create_codec()) {
      return false;
    }

    _decoding_thread = std::thread([this]() {
      decoding_loop();
    });
    _decoding_thread.detach();

    return true;
  }

  void seek(double target) {
    _cmds.push([this, target]() {
      avcodec_flush_buffers(_codec_ctx);
      av_seek_frame(_format_ctx, _stream->index, static_cast<long>(target / _time_base_sec / 1000.0), AVSEEK_FLAG_BACKWARD);

      skip_output_until_time = target;

      state = DecoderState::Ready;
    });
  }

  std::vector<DecodedFrame> get_decoded_frames() {
    std::vector<DecodedFrame> result;

    DecodedFrame out;
    while (_decoded_frames.try_pop(out)) {
      result.emplace_back(out);
    }

    return result;
  }

  ~VideoDecoder() {
    _stop_decoding_loop = true;

    //_thread_state

    sws_free_context(&sws_ctx);
    avformat_free_context(_format_ctx);
    avcodec_free_context(&_codec_ctx);

    // Will be freed by context frees
    _stream = nullptr;
    _codec = nullptr;
  }

  SwsContext *sws_ctx;

  DecoderState state;

  bool looping = false;

  double duration;
  double last_decoded_frame_time;
  std::optional<double> skip_output_until_time;

 private:
  void decoding_loop() {
    const auto packet = av_packet_alloc();
    const auto recv_frame = av_frame_alloc();

    const auto max_queued_frames = 3;

    while (!_stop_decoding_loop) {
      switch (state) {
        case DecoderState::Ready:
        case DecoderState::Running:
          if (_decoded_frames.unsafe_size() < max_queued_frames) {
            decode_next_frame(packet, recv_frame);
          } else {
            state = DecoderState::Ready;
            std::this_thread::sleep_for(1ms);
          }
          break;

        case DecoderState::EndOfStream:
          std::this_thread::sleep_for(50ms);
          break;
        default:
          return;
      }

      std::function<void()> cmd;
      while (_cmds.try_pop(cmd)) {
        if (cmd) {
          cmd();
        }
      }
    }
  }

  void decode_next_frame(AVPacket *packet, AVFrame *recv_frame) {
    auto result = 0;
    if (packet->buf == nullptr) {
      result = av_read_frame(_format_ctx, packet);
    }

    if (result >= 0) {
      state = DecoderState::Running;

      auto unref_packet = true;

      if (packet->stream_index == _stream->index) {
        if (send_packet(packet, recv_frame) == AVERROR(EAGAIN)) {
          unref_packet = false;
        }
      }

      if (unref_packet) {
        av_packet_unref(packet);
      }
    } else if (result == AVERROR_EOF) {
      send_packet(nullptr, recv_frame);

      if (looping) {
        seek(0);
      } else {
        state = DecoderState::EndOfStream;
      }
    } else if (result == AVERROR(EAGAIN)) {
      state = DecoderState::Ready;

      std::this_thread::sleep_for(1ms);
    } else {
      std::this_thread::sleep_for(1ms);
    }
  }

  int32_t send_packet(AVPacket *packet, AVFrame *recv_frame) {
    const auto result = avcodec_send_packet(_codec_ctx, packet);

    if (result == 0 || result == AVERROR(EAGAIN)) {
      read_decoded_frames(recv_frame);
    }

    return result;
  }

  void read_decoded_frames(AVFrame *recv_frame) {
    while (true) {
      const auto result = avcodec_receive_frame(_codec_ctx, recv_frame);

      if (result < 0) {
        break;
      }

      const auto frame_ts = recv_frame->best_effort_timestamp != AV_NOPTS_VALUE ? recv_frame->best_effort_timestamp : recv_frame->pts;
      const auto frame_time = (frame_ts - _stream->start_time) * _time_base_sec * 1000.0;

      if (skip_output_until_time.has_value() && skip_output_until_time.value() > frame_time) {
        continue;
      }

      auto frame = av_frame_alloc();
      av_frame_move_ref(frame, recv_frame);

      last_decoded_frame_time = frame_time;

      _decoded_frames.push({.time = frame_time, .frame = frame});
    }
  }

  bool prepare() {
    if (avformat_open_input(&_format_ctx, _path.c_str(), nullptr, nullptr) < 0) {
      return false;
    }

    if (avformat_find_stream_info(_format_ctx, nullptr) < 0) {
      return false;
    }

    const auto stream_index = av_find_best_stream(_format_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (stream_index < 0) {
      return false;
    }

    _stream = _format_ctx->streams[stream_index];
    _time_base_sec = av_q2d(_stream->time_base);

    if (_stream->duration > 0) {
      duration = _stream->duration * _time_base_sec * 1000.0;
    } else {
      duration = static_cast<double>(_format_ctx->duration) / AV_TIME_BASE * 1000.0;
    }

    return true;
  }

  bool create_codec() {
    if (!_stream) {
      return false;
    }

    const auto codec_params = *_stream->codecpar;

    _codec = avcodec_find_decoder(codec_params.codec_id);
    _codec_ctx = avcodec_alloc_context3(_codec);
    if (!_codec_ctx) {
      return false;
    }

    _codec_ctx->pkt_timebase = _stream->time_base;

    if (avcodec_parameters_to_context(_codec_ctx, &codec_params) < 0) {
      return false;
    }

    if (avcodec_open2(_codec_ctx, _codec, nullptr) < 0) {
      return false;
    }

    sws_ctx =
      sws_getContext(_codec_ctx->width, _codec_ctx->height, _codec_ctx->pix_fmt, _width, _height, AV_PIX_FMT_RGBA, SWS_BILINEAR, NULL, NULL, NULL);

    return true;
  }

  std::atomic<bool> _stop_decoding_loop = false;

  std::thread _decoding_thread;

  std::string _path;

  uint32_t _width;
  uint32_t _height;

  AVFormatContext *_format_ctx{};
  AVCodecContext *_codec_ctx{};

  const AVCodec *_codec;

  AVStream *_stream;

  double _time_base_sec;

  Concurrency::concurrent_queue<DecodedFrame> _decoded_frames;
  Concurrency::concurrent_queue<std::function<void()>> _cmds;
};
