#pragma once

#include "./RenderableModel.h"

struct GraphicsImpl;

class Graphics {
	GraphicsImpl* _impl;

	Graphics(const Graphics& other) = delete;
	inline Graphics(GraphicsImpl* impl) : _impl{impl} {}
public:
	static Graphics start(const RenderableModel& renderable_model);
	bool window_should_close();
	void render(float time);
	~Graphics();
};
