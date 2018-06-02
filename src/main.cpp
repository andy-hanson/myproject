#include <cassert>
#include <iostream> // std::cerr
#include <string>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <limits>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <sstream>

#include "./util/io.h"
#include "./util/float.h"
#include "./util/Matrix.h"
#include "./util/read_obj.h"
#include "./model.h"
#include "./process_model.h"

#include <soundio/soundio.h>

#include <cassert>
#include <sndfile.h>
#include <cstdint>
#include <iostream>
#include <vector>
#include <thread>

#include "./vendor/readerwriterqueue/readerwriterqueue.h"
#include "./shaders.h"

#include "vorbis/codec.h"
#include "vorbis/vorbisfile.h"

namespace {
	//TODO:MOVE
	__attribute__((noreturn))
	void todo() {
		throw "todo";
	}

	void init_glew() {
		glewExperimental = GL_TRUE;
		// Must be done after glut is initialized!
		GLenum res = glewInit();
		if (res != GLEW_OK) {
			std::cerr << "Error: " << glewGetErrorString(res) << std::endl;
			assert(false);
		}
	}

	GLFWwindow* init_glfw() {
		glfwInit();
		GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL", nullptr, nullptr);
		assert(window != nullptr);
		glfwMakeContextCurrent(window);
		return window;
	}

	void play_game(const Model& model) {
		RenderableModel renderable_model = convert_model(model);

		GLFWwindow* window = init_glfw();
		init_glew();

		VAO vao_tris = create_and_bind_vao();
		VBO vbo_tris = create_and_bind_vertex_buffer(renderable_model.tris);
		Shaders shaders_tri = init_shaders("tri");
		VAOInfo vao_info_tris { vao_tris, vbo_tris, shaders_tri, Uniforms { get_uniform(shaders_tri, "u_transform") } };

		VAO vao_dots = create_and_bind_vao();
		VBO vbo_dots = create_and_bind_vertex_buffer(renderable_model.dots);
		Shaders shaders_dot = init_shaders("dot");
		VAOInfo vao_info_dots { vao_dots, vbo_dots, shaders_dot, Uniforms { get_uniform(shaders_dot, "u_transform") } };

		glEnable(GL_DEPTH_TEST); //TODO: do this closer to where it's needed, and pair with glDIsable
		glEnable(GL_STENCIL_TEST);
		glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

		//load_texture();
		//glUniform1i(get_uniform(shaders, "tex").id, 0);

		float time = 0;
		while (!glfwWindowShouldClose(window)) {
			time += 0.01;
			render(vao_info_tris, vao_info_dots, time);
			glfwSwapBuffers(window);
			glfwPollEvents();
		}

		//TODO:DTOR
		free_shaders(shaders_tri);
		free_shaders(shaders_dot);

		//TODO: free VAO

		glfwDestroyWindow(window);
		glfwTerminate();
	}

	__attribute__((unused))
	void game() {
		std::string cwd = get_current_directory();
		std::string mtl_source = read_file(cwd + "/models/cube2.mtl");
		std::string obj_source = read_file(cwd + "/models/cube2.obj");
		Model m = parse_model(mtl_source.c_str(), obj_source.c_str());
		//gen_strokes(m);

		play_game(m);
	}
}


namespace {
	void handle_soundio_err(int err) {
		if (err) {
			std::cerr << soundio_strerror(err) << std::endl;
			assert(false);
		}
	}

	template <typename T>
	T* assert_not_null(T* ptr) {
		assert(ptr != nullptr);
		return ptr;
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
}

// NOTE: This runs in its own thread.
// AudioThreadData will be read by that thread and written by class Audio.
// Ignore frame_count_min, we should always write as much as possible.
static void write_callback(struct SoundIoOutStream *outstream, int frame_count_min __attribute__((unused)), int frame_count_max) {
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

struct PlayCommand {
	Slice<float> to_play;
};

class SoundCommand {
public:
	enum class Kind { Nil, Play, Stop };

private:
	union Data {
		PlayCommand play;

		Data() {}
		~Data() {}
	};
	Kind _kind;
	Data _data;

public:
	inline Kind kind() { return _kind; }

	inline SoundCommand() : _kind{Kind::Nil} {}
	SoundCommand(const SoundCommand& other) { *this = other; }
	void operator=(const SoundCommand& other) {
		_kind = other._kind;
		switch (other._kind) {
			case Kind::Nil:
			case Kind::Stop:
				break;
			case Kind::Play:
				_data.play = other._data.play; break;
		}

	}
	inline explicit SoundCommand(PlayCommand play) : _kind{Kind::Play} { _data.play = play; }
	inline explicit SoundCommand(Kind kind) : _kind{kind} { assert(_kind == Kind::Stop); }

	inline const PlayCommand& play() { assert(_kind == Kind::Play); return _data.play; }
};

namespace {
	class Audio {
		SoundIo* soundio;
		SoundIoDevice* device;
		SoundIoOutStream* outstream;

	public:
		void start() {
			soundio = assert_not_null(soundio_create());

			handle_soundio_err(soundio_connect(soundio));

			soundio_flush_events(soundio);

			int default_out_device_index = soundio_default_output_device_index(soundio);
			assert(default_out_device_index >= 0);

			device = assert_not_null(soundio_get_output_device(soundio, default_out_device_index));
			// std::cout << "Output device: " << device->name << std::endl;

			outstream = assert_not_null(soundio_outstream_create(device));
			outstream->format = SoundIoFormatFloat32NE;
			//TODO: does this launch a thread???
			outstream->write_callback = write_callback;

			handle_soundio_err(soundio_outstream_open(outstream));
			handle_soundio_err(outstream->layout_error);
			handle_soundio_err(soundio_outstream_start(outstream));
		}

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

	/*void audio_thread(moodycamel::ReaderWriterQueue<SoundCommand>* read_queue) {

		/ *std::cout << "audio_thread: " << std::this_thread::get_id() << std::endl;

		for (;;) {
			SoundCommand command;
			bool dequeued = read_queue->try_dequeue(command); // writes to command
			assert(dequeued == (command.kind() != SoundCommand::Kind::Nil));
			switch (command.kind()) {
				case SoundCommand::Kind::Nil:
					std::this_thread::sleep_for(std::chrono::milliseconds{10});
					break;
			}
		}* /
	}*/

	struct DecodedFile {
		// Note: this should be 2 channels stored together, L R L R L R L R.
		DynArray<float> floats;
	};

	DecodedFile read_wav() {
		SF_INFO sf_info;
		// See http://www.mega-nerd.com/libsndfile/api.html#open
		// The format field should be set to zero for some reason. Other than that, info is written to by `sf_open`, not read by it.
		sf_info.format = 0;
		SNDFILE* f = sf_open("/home/andy/CLionProjects/myproject/audio/awe.wav", SFM_READ, &sf_info);
		assert(f); //else file did not exist
		assert(sf_info.channels == 2);

		DecodedFile res { { i64_to_u32(sf_info.frames * sf_info.channels) } };

		sf_count_t n_read = sf_read_float(f, &res.floats[0], res.floats.size());
		assert(n_read == res.floats.size());

		sf_close(f);

		return res;
	}

	DecodedFile read_vorbis(const char* file_name) {
		OggVorbis_File vf;
		FILE* file = fopen(file_name, "r");
		if (file == nullptr) {
			perror("error opening file");
			assert(false);
		}
		int err = ov_open(file, &vf, nullptr, 0);
		assert(err == 0);

		// coments
		{
			char** ptr = ov_comment(&vf, -1)->user_comments;
			vorbis_info* vi = ov_info(&vf, -1);
			while (*ptr) {
				fprintf(stderr,"%s\n",*ptr);
				++ptr;
			}
			fprintf(stderr,"\nBitstream is %d channel, %ldHz\n",vi->channels,vi->rate);
			fprintf(stderr,"\nDecoded length: %ld samples\n", ov_pcm_total(&vf,-1));
			fprintf(stderr,"Encoded by: %s\n\n",ov_comment(&vf,-1)->vendor);
		}

		//TODO: get length ahead of time?
		std::vector<float> res;
		res.reserve(10000);

		for (;;) {
			const uint N_shorts = 4096;
			i16 buf[N_shorts];
			static_assert(sizeof(i16) == sizeof(char) * 2);
			uint word_size = sizeof(i16) / sizeof(char);
			uint N_chars = N_shorts * word_size;
			int current_section;
			long ret = ov_read(
				&vf,
				reinterpret_cast<char*>(buf),
				uint_to_int(N_shorts * word_size),
				/*bigendian*/ false,
				uint_to_int(word_size),
				/*signed*/ true,
				&current_section);
			if (ret == 0) {
				/* EOF */
				break;
			} else if (ret < 0) {
				// error in the stream
				assert(false);
			} else {
				if (current_section != 0) {
					// we don't bother dealing with sample rate changes, etc, but you'll have to
					todo();
				}

				// Note: "ov_read() will decode at most one vorbis packet per invocation, so the value returned will generally be less than length."
				uint n_chars_read = long_to_uint(ret);
				uint n_shorts_read = safe_div(n_chars_read, word_size);
				assert(n_chars_read < N_chars);

				const float AMPLITUDE = std::numeric_limits<i16>::max();
				for (uint i = 0; i != n_shorts_read; ++i) {
					float f = static_cast<float>(buf[i]) / AMPLITUDE;
					res.push_back(f);
				}
			}
		}

		ov_clear(&vf);

		return { vec_to_dyn_array(res) };
	}

	template <typename Cb>
	auto foo(const char* desc, Cb cb) {
		std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
		auto res = cb();
		std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();

		std::cout << desc << " took " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
		return res;
	}
}

//TODO: now play the wav data into soundio...

int main() {
	DecodedFile wavvy = foo("wav", []() { return read_wav(); });
	DecodedFile vorby = foo("ogg", []() { return read_vorbis("/home/andy/CLionProjects/myproject/audio/awe.ogg"); });

	bool prefer_wav = false;

	Audio audio;
	audio.start();
	audio.play(prefer_wav ? wavvy.floats.slice() : vorby.floats.slice());
	std::this_thread::sleep_for(std::chrono::seconds{10});
	audio.stop();



	/*
	OLD:

	moodycamel::ReaderWriterQueue<SoundCommand> q;
	std::thread aud { audio_thread, &q };

	q.emplace(SoundCommand { PlayCommand { floats.slice() } });

	std::cout << "sleep" << std::endl;

	std::this_thread::sleep_for(std::chrono::seconds{10});

	std::cout << "woke" << std::endl;

	q.emplace(SoundCommand { SoundCommand::Kind::Stop });
	aud.join();*/

}
