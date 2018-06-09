#include "glm/vec2.hpp"

#include "./util/FixedArray.h"
#include "./util/io.h"
#include "./util/Ref.h"
#include "./graphics/Graphics.h"
#include "./model/Model.h"
#include "./model/ModelKind.h"
#include "./model/parse_model.h"
#include "./physics/Physics.h"

#include "./game.h"

namespace {
	void play_game(Graphics& graphics, Physics& physics __attribute__((unused))) {
		std::vector<DrawEntity> draw;
		draw.push_back(DrawEntity { ModelKind::Player, Transform { glm::vec3(0.0f), glm::quat{} } });
		draw.push_back(DrawEntity { ModelKind::Cylinder, Transform { glm::vec3(0.0f), glm::quat{} } });

		while (!graphics.window_should_close()) {
			graphics.render(vec_to_slice(draw));
		}
	}

	//TODO:MOVE
	using AllModels = FixedArray<N_MODELS, Model>;
	AllModels load_all_models(const std::string& cwd) {
		std::string models = cwd + "/models/";
		auto foo = [&](const char* name) {
			std::string mtl_source = read_file(models + name + ".mtl");
			std::string obj_source = read_file(models + name + ".obj");
			return parse_model(mtl_source.c_str(), obj_source.c_str());
		};

		FixedArray<N_MODELS, Model> res;

		res[0] = foo("player");
		res[1] = foo("cylinder");

		assert(N_MODELS == 2);

		return res;
	}
}

void game(const std::string& cwd) {
	AllModels all_models = load_all_models(cwd);
	Graphics graphics = Graphics::start(all_models.slice(), cwd);
	Physics physics { all_models.slice() };
	play_game(graphics, physics);
}
