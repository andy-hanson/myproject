#pragma once

#include "glm/vec2.hpp"

struct ControllerImpl;

struct ControllerGet {
	glm::vec2 joy;
	bool button_is_down;
	bool button_just_pressed; // True if wasn't pressed before
	bool start_just_pressed;
};

class Controller {
	ControllerImpl* impl;
	inline Controller(ControllerImpl* _impl) : impl{_impl} {}

public:
	static Controller start();
	ControllerGet get();
	~Controller();
};
