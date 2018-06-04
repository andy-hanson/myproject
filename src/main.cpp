#include <GL/glew.h>
#include <iostream>
#include <thread>

#include "./util/io.h"
#include "./util/float.h"
#include "./util/Matrix.h"

#include "./graphics/process_model.h"
#include "./graphics/model.h"
#include "./graphics/read_obj.h"
#include "./graphics/shaders.h"

#include "./vendor/readerwriterqueue/readerwriterqueue.h"

#include "./audio/audio.h"
#include "./audio/audio_file.h"
#include "./audio/read_wav.h"
#include "./audio/read_ogg.h"
#include "./control/Controller.h"

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

namespace {

	void test_input() {
		Controller controller = Controller::start();
		for (uint i = 0; i != 100; ++i) {
			ControllerGet g = controller.get();
			std::cout << g.button_is_down << std::endl;
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}
}


int main() {
	if ((false)) test_sound();
	if ((false)) game();
	if ((false)) test_input();
}
