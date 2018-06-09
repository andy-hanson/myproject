#pragma once

#include <glm/vec2.hpp>
#include <string>

#include "../model/Model.h"
#include "../model/ModelKind.h"

/** This is the input to the graphics system. */
struct DrawEntity {
	ModelKind model;
	glm::vec2 position;
	//rp3d::Quaternion orientation;
};

struct GraphicsImpl;

class Graphics {
	GraphicsImpl* _impl;

	Graphics(const Graphics& other) = delete;
	inline Graphics(GraphicsImpl* impl) : _impl{impl} {}
public:
	static Graphics start(const Model& renderable_model, const std::string& cwd);
	bool window_should_close();
	void render(Slice<DrawEntity> to_draw);
	~Graphics();
};
