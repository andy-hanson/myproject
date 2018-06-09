#include "glm/vec2.hpp"

#include "./util/io.h"
#include "./util/Ref.h"
#include "./graphics/Graphics.h"
#include "./model/Model.h"
#include "./model/ModelKind.h"
#include "./model/parse_model.h"

#include "./game.h"

namespace {
	void play_game(Graphics& graphics) {
		std::vector<DrawEntity> draw;
		draw.push_back(DrawEntity { ModelKind::Player, glm::vec2(0.0f, 0.0f) });

		while (!graphics.window_should_close()) {
			graphics.render(vec_to_slice(draw));
		}
	}
}

void game(const std::string& cwd) {
	std::string mtl_source = read_file(cwd + "/models/cube2.mtl");
	std::string obj_source = read_file(cwd + "/models/cube2.obj");
	Model model = parse_model(mtl_source.c_str(), obj_source.c_str());
	Graphics graphics = Graphics::start(model, cwd);
	play_game(graphics);
}
