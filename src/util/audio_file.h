#pragma once

#include "./DynArray.h"

struct DecodedAudioFile {
	// Note: this should be 2 channels stored together, L R L R L R L R.
	DynArray<float> floats;
};
