#include "BinkVideoDecoder.h"
#include "X2DDisplay.h"
#include <Windows.h>
#include <mmsystem.h>
#include <algorithm>
#include <cerrno>
#include <cmath>
#include <deque>
#include <limits>
#include <new>
#include <string>
#include <vector>

#pragma warning(push)
#pragma warning(disable: 4244) // Narrowing in FFmpeg's own inline numeric helpers.
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}
#pragma warning(pop)

namespace NWindowsVideo
{
	struct CBinkVideoDecoder::SPlayback
	{
		struct SFrameDeleter { void operator()(AVFrame* frame) const { av_frame_free(&frame); } };
		using FramePtr = std::unique_ptr<AVFrame, SFrameDeleter>;
		struct SVideoFrame { FramePtr frame; double time; };
		struct SAudioBuffer
		{
			WAVEHDR header{};
			std::vector<std::uint8_t> samples;
			bool prepared = false;
		};
		AVFormatContext* format = nullptr;
		AVCodecContext* video = nullptr;
		AVCodecContext* audio = nullptr;
		AVPacket* packet = nullptr;
		AVFrame* frame = nullptr;
		SwsContext* scaler = nullptr;
		SwrContext* resampler = nullptr;
		HWAVEOUT wave = nullptr;
		std::deque<SVideoFrame> frames;
		std::deque<std::unique_ptr<SAudioBuffer>> sound;
		std::vector<std::uint8_t> pixels;
		int videoIndex = -1, audioIndex = -1;
		int sampleRate = 0, channels = 0;
		int width = 0, height = 0;
		double frameDuration = 0, nextFrameTime = 0, videoEnd = 0;
		LARGE_INTEGER frequency{}, lastTick{};
		double clockTime = 0;
		std::size_t queuedAudioBytes = 0;
		bool playing = false, eof = false, started = false;
		static constexpr double DecodeAheadSeconds = 0.25;
		static constexpr std::size_t MaxQueuedFrames = 32;
		static constexpr std::size_t MaxQueuedVideoBytes = 64 * 1024 * 1024;

		~SPlayback()
		{
			CloseAudio();
			swr_free(&resampler);
			sws_freeContext(scaler);
			av_frame_free(&frame);
			av_packet_free(&packet);
			avcodec_free_context(&audio);
			avcodec_free_context(&video);
			avformat_close_input(&format);
		}

		void ClearSound()
		{
			if (wave != nullptr) waveOutReset(wave);
			for (auto& buffer : sound) if (buffer->prepared) waveOutUnprepareHeader(wave, &buffer->header, sizeof(WAVEHDR));
			sound.clear();
			queuedAudioBytes = 0;
		}

		void CloseAudio()
		{
			ClearSound();
			if (wave != nullptr) waveOutClose(wave);
			wave = nullptr;
		}

		bool OpenCodec(int index, AVCodecContext*& context)
		{
			const auto* parameters = format->streams[index]->codecpar;
			const AVCodec* codec = avcodec_find_decoder(parameters->codec_id);
			if (codec == nullptr) return false;
			context = avcodec_alloc_context3(codec);
			return context != nullptr && avcodec_parameters_to_context(context, parameters) >= 0 && avcodec_open2(context, codec, nullptr) >= 0;
		}

		void OpenAudio()
		{
			// Bink's first audio track matches the previous default track selection.
			for (unsigned i = 0; i < format->nb_streams; ++i)
			{
				const auto id = format->streams[i]->codecpar->codec_id;
				if (id == AV_CODEC_ID_BINKAUDIO_DCT || id == AV_CODEC_ID_BINKAUDIO_RDFT) { audioIndex = static_cast<int>(i); break; }
			}
			if (audioIndex < 0 || !OpenCodec(audioIndex, audio)) return;
			if (audio->sample_rate < 8000 || audio->sample_rate > 192000 || audio->ch_layout.nb_channels < 1 || audio->ch_layout.nb_channels > 8) return;
			sampleRate = audio->sample_rate;
			channels = (std::min)(audio->ch_layout.nb_channels, 2);
			AVChannelLayout output{};
			av_channel_layout_default(&output, channels);
			const int result = swr_alloc_set_opts2(&resampler, &output, AV_SAMPLE_FMT_S16, sampleRate, &audio->ch_layout, audio->sample_fmt, sampleRate, 0, nullptr);
			av_channel_layout_uninit(&output);
			if (result < 0 || swr_init(resampler) < 0) return;
			WAVEFORMATEX pcm{};
			pcm.wFormatTag = WAVE_FORMAT_PCM;
			pcm.nChannels = static_cast<WORD>(channels);
			pcm.nSamplesPerSec = static_cast<DWORD>(sampleRate);
			pcm.wBitsPerSample = 16;
			pcm.nBlockAlign = static_cast<WORD>(channels * 2);
			pcm.nAvgBytesPerSec = pcm.nSamplesPerSec * pcm.nBlockAlign;
			if (waveOutOpen(&wave, WAVE_MAPPER, &pcm, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) wave = nullptr;
			if (wave != nullptr && waveOutPause(wave) != MMSYSERR_NOERROR) CloseAudio();
		}

		bool Open(const char* fileName, bool enableAudio)
		{
			// The file resolver returns ACP paths; libavformat expects UTF-8 on Windows.
			const int count = MultiByteToWideChar(CP_ACP, 0, fileName, -1, nullptr, 0);
			if (count <= 0) return false;
			std::vector<wchar_t> wide(static_cast<std::size_t>(count));
			if (MultiByteToWideChar(CP_ACP, 0, fileName, -1, wide.data(), count) == 0) return false;
			const int bytes = WideCharToMultiByte(CP_UTF8, 0, wide.data(), -1, nullptr, 0, nullptr, nullptr);
			if (bytes <= 0) return false;
			std::string utf8(static_cast<std::size_t>(bytes), '\0');
			if (WideCharToMultiByte(CP_UTF8, 0, wide.data(), -1, utf8.data(), bytes, nullptr, nullptr) == 0) return false;
			const AVInputFormat* bink = av_find_input_format("bink");
			if (bink == nullptr || avformat_open_input(&format, utf8.c_str(), bink, nullptr) < 0 || avformat_find_stream_info(format, nullptr) < 0) return false;
			videoIndex = av_find_best_stream(format, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
			if (videoIndex < 0 || format->streams[videoIndex]->codecpar->codec_id != AV_CODEC_ID_BINKVIDEO || !OpenCodec(videoIndex, video)) return false;
			width = video->width; height = video->height;
			if (width <= 0 || height <= 0 || !NXSys2DApp::CDisplay::IsSupportedMode(static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height), 32)) return false;
			const auto pixelBytes = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4;
			if (width > (std::numeric_limits<int>::max)() / 4 || pixelBytes > MaxQueuedVideoBytes) return false;
			pixels.resize(pixelBytes);
			const AVRational rate = av_guess_frame_rate(format, format->streams[videoIndex], nullptr);
			if (rate.num <= 0 || rate.den <= 0) return false;
			frameDuration = av_q2d(av_inv_q(rate));
			if (!std::isfinite(frameDuration) || frameDuration <= 0) return false;
			packet = av_packet_alloc(); frame = av_frame_alloc();
			if (packet == nullptr || frame == nullptr || !QueryPerformanceFrequency(&frequency) || frequency.QuadPart <= 0) return false;
			if (enableAudio)
			{
				OpenAudio();
				if (wave == nullptr)
				{
					audioIndex = -1;
					swr_free(&resampler);
					avcodec_free_context(&audio);
					OutputDebugStringA("OpenVikings: Bink audio unavailable; playing silently.\n");
				}
			}
			return true;
		}

		void ReapSound()
		{
			while (!sound.empty() && (sound.front()->header.dwFlags & WHDR_DONE) != 0)
			{
				if (waveOutUnprepareHeader(wave, &sound.front()->header, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) break;
				queuedAudioBytes -= sound.front()->header.dwBufferLength;
				sound.pop_front();
			}
		}

		bool QueueAudio(const AVFrame* decoded)
		{
			ReapSound();
			// Bound corrupt/interleaved inputs rather than growing PCM queues indefinitely.
			if (sound.size() >= 128 || decoded->nb_samples <= 0 || decoded->sample_rate != sampleRate || av_channel_layout_compare(&decoded->ch_layout, &audio->ch_layout) != 0 || decoded->format != audio->sample_fmt) return false;
			const int capacity = swr_get_out_samples(resampler, decoded->nb_samples);
			if (capacity <= 0 || capacity > sampleRate * 2) return false;
			auto buffer = std::make_unique<SAudioBuffer>();
			buffer->samples.resize(static_cast<std::size_t>(capacity) * channels * 2);
			std::uint8_t* output[] = { buffer->samples.data() };
			const std::uint8_t* input[8]{};
			const int planes = av_sample_fmt_is_planar(audio->sample_fmt) ? audio->ch_layout.nb_channels : 1;
			for (int i = 0; i < planes; ++i) input[i] = decoded->extended_data[i];
			const int count = swr_convert(resampler, output, capacity, input, decoded->nb_samples);
			if (count < 0) return false;
			if (count == 0) return true;
			buffer->header.lpData = reinterpret_cast<LPSTR>(buffer->samples.data());
			buffer->header.dwBufferLength = static_cast<DWORD>(count * channels * 2);
			if (queuedAudioBytes + buffer->header.dwBufferLength > static_cast<std::size_t>(sampleRate) * channels * 4) return false;
			// Publish stable ownership before exposing the buffer to the Windows device.
			sound.push_back(std::move(buffer));
			auto& queued = *sound.back();
			if (waveOutPrepareHeader(wave, &queued.header, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) { sound.pop_back(); return false; }
			queued.prepared = true;
			queuedAudioBytes += queued.header.dwBufferLength;
			return waveOutWrite(wave, &queued.header, sizeof(WAVEHDR)) == MMSYSERR_NOERROR;
		}

		bool Receive(AVCodecContext* codec)
		{
			for (;;)
			{
				const int result = avcodec_receive_frame(codec, frame);
				if (result == AVERROR(EAGAIN) || result == AVERROR_EOF) return true;
				if (result < 0) return false;
				bool accepted = true;
				if (codec == video)
				{
					if (frame->width != width || frame->height != height || frames.size() >= MaxQueuedFrames || (frames.size() + 1) * pixels.size() > MaxQueuedVideoBytes) accepted = false;
					else
					{
						const AVStream* stream = format->streams[videoIndex];
						double time = nextFrameTime;
						if (frame->best_effort_timestamp != AV_NOPTS_VALUE)
						{
							const auto origin = stream->start_time == AV_NOPTS_VALUE ? 0 : stream->start_time;
							time = (static_cast<double>(frame->best_effort_timestamp) - static_cast<double>(origin)) * av_q2d(stream->time_base);
						}
						if (!std::isfinite(time) || time < 0 || time + 0.001 < nextFrameTime - frameDuration) accepted = false;
						else
						{
							FramePtr copy(av_frame_clone(frame));
							if (!copy) accepted = false;
							else { frames.push_back({ std::move(copy), time }); nextFrameTime = time + frameDuration; videoEnd = nextFrameTime; }
						}
					}
				}
				else accepted = QueueAudio(frame);
				av_frame_unref(frame);
				if (!accepted) return false;
			}
		}

		bool Submit(AVCodecContext* codec, const AVPacket* input)
		{
			int result = avcodec_send_packet(codec, input);
			if (result == AVERROR(EAGAIN))
			{
				if (!Receive(codec)) return false;
				result = avcodec_send_packet(codec, input);
			}
			return (result >= 0 || result == AVERROR_EOF) && Receive(codec);
		}

		bool Pump(double time)
		{
			for (unsigned work = 0; work < 512 && !eof && (frames.empty() || frames.back().time < time + DecodeAheadSeconds); ++work)
			{
				if (frames.size() >= MaxQueuedFrames || (frames.size() + 1) * pixels.size() > MaxQueuedVideoBytes) break;
				const int result = av_read_frame(format, packet);
				if (result == AVERROR_EOF)
				{
					eof = true;
					return Submit(video, nullptr) && (wave == nullptr || Submit(audio, nullptr));
				}
				if (result < 0) return false;
				bool decoded = true;
				if (packet->stream_index == videoIndex) decoded = Submit(video, packet);
				else if (packet->stream_index == audioIndex && wave != nullptr) decoded = Submit(audio, packet);
				av_packet_unref(packet);
				if (!decoded) return false;
			}
			return true;
		}

		bool Start()
		{
			playing = false;
			ClearSound();
			frames.clear();
			if (started && av_seek_frame(format, videoIndex, 0, AVSEEK_FLAG_BACKWARD) < 0) return false;
			started = true;
			avcodec_flush_buffers(video);
			if (audio != nullptr) avcodec_flush_buffers(audio);
			if (resampler != nullptr) { swr_close(resampler); if (swr_init(resampler) < 0) return false; }
			eof = false; nextFrameTime = 0; videoEnd = 0;
			if (wave != nullptr && waveOutPause(wave) != MMSYSERR_NOERROR) return false;
			if (!Pump(0) || frames.empty()) return false;
			clockTime = 0;
			QueryPerformanceCounter(&lastTick);
			if (wave != nullptr && waveOutRestart(wave) != MMSYSERR_NOERROR) return false;
			playing = true;
			return true;
		}

		void Stop()
		{
			playing = false;
			if (wave != nullptr) waveOutPause(wave);
		}

		bool Update(NXSys2DApp::CDisplay* display)
		{
			if (!playing || display == nullptr) return false;
			LARGE_INTEGER now{};
			QueryPerformanceCounter(&now);
			ReapSound();
			double time = clockTime + static_cast<double>(now.QuadPart - lastTick.QuadPart) / static_cast<double>(frequency.QuadPart);
			if (wave != nullptr && !sound.empty())
			{
				// Device progress keeps the picture synchronized through slow decode/update calls.
				MMTIME position{};
				position.wType = TIME_SAMPLES;
				if (waveOutGetPosition(wave, &position, sizeof(position)) == MMSYSERR_NOERROR)
				{
					if (position.wType == TIME_SAMPLES) time = static_cast<double>(position.u.sample) / sampleRate;
					else if (position.wType == TIME_BYTES) time = static_cast<double>(position.u.cb) / (sampleRate * channels * 2);
					else if (position.wType == TIME_MS) time = static_cast<double>(position.u.ms) / 1000;
				}
			}
			time = (std::max)(clockTime, time);
			clockTime = time; lastTick = now;
			if (!Pump(time)) return false;
			FramePtr selected;
			while (!frames.empty() && frames.front().time <= time)
			{
				selected = std::move(frames.front().frame);
				frames.pop_front();
			}
			if (selected)
			{
				scaler = sws_getCachedContext(scaler, width, height, static_cast<AVPixelFormat>(selected->format), width, height, AV_PIX_FMT_BGRA, SWS_BILINEAR, nullptr, nullptr, nullptr);
				if (scaler == nullptr) return false;
				std::uint8_t* output[] = { pixels.data(), nullptr, nullptr, nullptr };
				int strides[] = { width * 4, 0, 0, 0 };
				if (sws_scale(scaler, selected->data, selected->linesize, 0, height, output, strides) != height) return false;
				if (!display->VideoFrameToScreen(pixels.data(), width * 4, width, height, 32, 0x00FF0000, 0x0000FF00, 0x000000FF)) return false;
			}
			// Retain the final frame for its full interval and let queued audio finish.
			if (eof && frames.empty() && sound.empty() && time >= videoEnd) Stop();
			return true;
		}
	};

	CBinkVideoDecoder::CBinkVideoDecoder() = default;
	CBinkVideoDecoder::~CBinkVideoDecoder() = default;

	bool CBinkVideoDecoder::Open(const char* fileName, bool enableAudio)
	{
		mPlayback.reset();
		if (fileName == nullptr) return false;
		try
		{
			auto playback = std::make_unique<SPlayback>();
			if (!playback->Open(fileName, enableAudio)) return false;
			mPlayback = std::move(playback);
			return true;
		}
		catch (const std::bad_alloc&) { return false; }
	}

	bool CBinkVideoDecoder::Start()
	{
		try { if (mPlayback && mPlayback->Start()) return true; }
		catch (const std::bad_alloc&) { }
		Stop();
		return false;
	}

	void CBinkVideoDecoder::Stop()
	{
		if (mPlayback) mPlayback->Stop();
	}

	bool CBinkVideoDecoder::Update(NXSys2DApp::CDisplay* display)
	{
		try { if (mPlayback && mPlayback->Update(display)) return true; }
		catch (const std::bad_alloc&) { }
		OutputDebugStringA("OpenVikings: Bink playback stopped after a decode/output failure.\n");
		Stop();
		return false;
	}

	bool CBinkVideoDecoder::IsPlaying() const
	{
		return mPlayback && mPlayback->playing;
	}
}
