#include "./read_wav.h"

#include <sndfile.h>
#include "./assert.h"

DecodedAudioFile read_wav() {
	SF_INFO sf_info;
	// See http://www.mega-nerd.com/libsndfile/api.html#open
	// The format field should be set to zero for some reason. Other than that, info is written to by `sf_open`, not read by it.
	sf_info.format = 0;
	SNDFILE* f = sf_open("/home/andy/CLionProjects/myproject/audio/awe.wav", SFM_READ, &sf_info);
	assert_not_null(f); //else file did not exist
	check(sf_info.channels == 2);

	DecodedAudioFile res { DynArray<float>::uninitialized(i64_to_u32(sf_info.frames * sf_info.channels)) };

	sf_count_t n_read = sf_read_float(f, &res.floats[0], res.floats.size());
	check(n_read == res.floats.size());

	sf_close(f);

	return res;
}
