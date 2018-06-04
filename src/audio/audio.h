#pragma once

#include "../util/Slice.h"

struct AudioImpl;
class Audio {
	AudioImpl* impl;
	inline Audio(AudioImpl* _impl) : impl{_impl} {}

public:
	static Audio start();
	void play(Slice<float> to_play);
	~Audio();
};
