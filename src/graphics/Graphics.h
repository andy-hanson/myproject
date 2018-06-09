#pragma once

#include <string>

#include "../util/Transform.h"
#include "../model/Model.h"
#include "../model/ModelKind.h"

/**
 * This is the input to the graphics system: Draw a model at a certain transform.
 * TODO: support more properties of objects.
 */
struct DrawEntity {
	ModelKind model;
	Transform transform;
};

struct GraphicsImpl;

class Graphics {
	GraphicsImpl* _impl;

	Graphics(const Graphics& other) = delete;
	inline Graphics(GraphicsImpl* impl) : _impl{impl} {}
public:
	static Graphics start(Slice<Model> models, const std::string& cwd);
	bool window_should_close();
	void render(Slice<DrawEntity> to_draw);
	~Graphics();
};
