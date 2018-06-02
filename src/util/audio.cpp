#include "./audio.h"

#include <soundio/soundio.h>
#include "./assert.h"
#include "./Slice.h"

namespace {

	void handle_soundio_err(int err) {
		if (err) {
			//std::cerr << soundio_strerror(err) << std::endl;
			todo();
		}
	}

	//Thread locals to the audio thread.
	struct AudioThreadData {
		float seconds_offset = 0.0f;
		Slice<float> playing; // may be empty
		u32 playing_index;
	};

	AudioThreadData& thread_local_audio_thread_data() {
		static AudioThreadData a;
		return a;
	}

	// NOTE: This runs in its own thread.
	// AudioThreadData will be read by that thread and written by class Audio.
	// Ignore frame_count_min, we should always write as much as possible.
	void write_callback(struct SoundIoOutStream *outstream, int frame_count_min __attribute__((unused)), int frame_count_max) {
		const struct SoundIoChannelLayout *layout = &outstream->layout;
		//float float_sample_rate = outstream->sample_rate;
		//float seconds_per_frame = 1.0f / float_sample_rate;
		struct SoundIoChannelArea *areas;

		int frames_left = frame_count_max;

		AudioThreadData& data = thread_local_audio_thread_data();

		//std::cout << "write_callback: " << std::this_thread::get_id() << std::endl;

		while (frames_left > 0) {
			int frame_count = frames_left;
			handle_soundio_err(soundio_outstream_begin_write(outstream, &areas, &frame_count));
			if (!frame_count)
				break;

			/*
			float pitch = 440.0f;
			float radians_per_second = pitch * 2.0f * 3.1415926535f;
			for (int frame = 0; frame < frame_count; frame += 1) {
				float volume = static_cast<float>(0.05);
				float sample = float_sin((data.seconds_offset + frame * seconds_per_frame) * radians_per_second) * volume;
				for (int channel = 0; channel < layout->channel_count; channel += 1) {
					float* ptr = reinterpret_cast<float*>(areas[channel].ptr + areas[channel].step * frame);
					*ptr = sample;
				}
			}
			data.seconds_offset = static_cast<float>(fmod(static_cast<double>(data.seconds_offset + seconds_per_frame * frame_count), 1.0));
			*/
			for (int frame = 0; frame < frame_count; ++frame) {
				for (int channel = 0; channel < layout->channel_count; channel += 1) {
					float sample;
					if (data.playing.is_empty()) {
						sample = 0;
					} else {
						// Samples for each channel come one after another.
						sample = data.playing[data.playing_index];
						++data.playing_index;
						if (data.playing_index == data.playing.size())
							data.playing_index = 0;

					}


					float* ptr = reinterpret_cast<float*>(areas[channel].ptr + areas[channel].step * frame);
					*ptr = sample;
				}
			}

			handle_soundio_err(soundio_outstream_end_write(outstream));
			frames_left -= frame_count;
		}
	}
}

struct AudioImpl {
	SoundIo* soundio;
	SoundIoDevice* device;
	SoundIoOutStream* outstream;

	void play(Slice<float> to_play) {
		AudioThreadData& data = thread_local_audio_thread_data();
		data.playing = to_play;
		data.playing_index = 0;
	}

	void stop() {
		soundio_outstream_destroy(outstream);
		soundio_device_unref(device);
		soundio_destroy(soundio);
	}
};

Audio Audio::start() {
	SoundIo* soundio = assert_not_null(soundio_create());

	handle_soundio_err(soundio_connect(soundio));

	soundio_flush_events(soundio);

	int default_out_device_index = soundio_default_output_device_index(soundio);
	check(default_out_device_index >= 0);

	SoundIoDevice* device = assert_not_null(soundio_get_output_device(soundio, default_out_device_index));
	// std::cout << "Output device: " << device->name << std::endl;

	SoundIoOutStream* outstream = assert_not_null(soundio_outstream_create(device));
	outstream->format = SoundIoFormatFloat32NE;
	//TODO: does this launch a thread???
	outstream->write_callback = write_callback;

	handle_soundio_err(soundio_outstream_open(outstream));
	handle_soundio_err(outstream->layout_error);
	handle_soundio_err(soundio_outstream_start(outstream));

	return Audio { new AudioImpl { soundio, device, outstream } };
}
void Audio::play(Slice<float> to_play) { impl->play(to_play); }
Audio::~Audio() {
	impl->stop();
	delete impl;
}
