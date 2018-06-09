#include <iostream>//TODO:KILL
#include "glm/vec2.hpp"

#include "./util/FixedArray.h"
#include "./util/io.h"
#include "./util/Ref.h"
#include "./control/Controller.h"
#include "./graphics/Graphics.h"
#include "./model/Model.h"
#include "./model/ModelKind.h"
#include "./model/parse_model.h"
#include "./physics/Physics.h"
#include "./Timer.h"

#include "./game.h"

namespace {
	struct GameState {
		glm::vec3 player;

		GameState() : player{0.0f} {}
	};

	const char* model_name(ModelKind kind) {
		switch (kind) {
			case ModelKind::Player:
				return "player";
			case ModelKind::Cylinder:
				return "cylinder";
			case ModelKind::COUNT:
				return nullptr;
		}
	}
	//TODO:MOVE
	DynArray<Model> load_all_models(const std::string& cwd) {
		std::string models = cwd + "/models/";
		return fill_array<Model>{}(N_MODELS, [&](u32 i) {
			ModelKind k = ModelKind(i);
			const char* name = model_name(k);
			std::string mtl_source = read_file(models + name + ".mtl");
			std::string obj_source = read_file(models + name + ".obj");
			return parse_model(mtl_source.c_str(), obj_source.c_str());
		});
	}

	struct Game {
		Timer timer;
		DynArray<Model> models;
		Graphics graphics;
		Physics physics;
		Controller controller;
		GameState state;

		Game(const std::string& cwd)
		: timer{},
			models{load_all_models(cwd)},
			graphics{Graphics::start(models.slice(), cwd)},
			physics { models.slice() },
			controller{Controller::start()},
			state {} {}
	};

	//TODO:MOVE
	__attribute__((unused))
	std::ostream& operator<<(std::ostream& out, const glm::vec2& v) {
		return out << "(" << v.x << ", " << v.y << ")";
	}

	void play_game(Game& game) {
		std::vector<DrawEntity> draw;
		draw.push_back(DrawEntity { ModelKind::Player, Transform { glm::vec3(0.0f), glm::quat{} } });
		draw.push_back(DrawEntity { ModelKind::Cylinder, Transform { glm::vec3(0.0f), glm::quat{} } });

		while (!game.graphics.window_should_close()) {
			double fps __attribute__((unused)) = game.timer.tick();
			//std::cout << "FPS: " << fps << std::endl;
			//std::cout << game.controller.get().joy << std::endl;

			draw[0].transform.position = glm::vec3 { game.controller.get().joy, 0.0f };

			game.graphics.render(vec_to_slice(draw));
		}
	}
}

void game(const std::string& cwd) {
	Game game { cwd };
	play_game(game);
}
