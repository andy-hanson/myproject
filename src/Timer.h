#pragma once

struct TimerImpl;

class Timer {
	TimerImpl* impl;

public:
	Timer();
	~Timer();

	double tick();
};
