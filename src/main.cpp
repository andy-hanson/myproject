#include <cassert>
#include <iostream> // std::cerr
#include <string>
#include <GL/glew.h>
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


#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>
#include <thread>

#include "./vendor/readerwriterqueue/readerwriterqueue.h"
#include "./shaders.h"

#include "./util/audio.h"
#include "./util/audio_file.h"
#include "./util/read_wav.h"
#include "./util/read_ogg.h"

namespace {

	void play_game(const Model& model) {
		RenderableModel renderable_model = convert_model(model);

		Graphics graphics = Graphics::start(renderable_model);

		float time = 0;
		while (!graphics.window_should_close()) {
			time += 0.01;
			graphics.render(time);
		}
	}

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

}


namespace {
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

	template <typename Cb>
	auto print_time(const char* desc, Cb cb) {
		std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
		auto res = cb();
		std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();

		std::cout << desc << " took " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
		return res;
	}

	void test_sound() {
		DecodedAudioFile wavvy = print_time("wav", []() { return read_wav(); });
		DecodedAudioFile vorby = print_time("ogg", []() { return read_ogg("/home/andy/CLionProjects/myproject/audio/awe.ogg"); });

		std::cout << "size: " << wavvy.floats.size() << "   " << vorby.floats.size() << std::endl;

		bool prefer_wav = false;

		Audio audio = Audio::start();
		audio.play(prefer_wav ? wavvy.floats.slice() : vorby.floats.slice());
		std::this_thread::sleep_for(std::chrono::seconds{5});
	}
}

//TODO: now play the wav data into soundio...

int main() {
	if ((false)) test_sound();

	game();

}
