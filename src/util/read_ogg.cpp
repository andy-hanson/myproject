#include "./read_ogg.h"

#include "vorbis/codec.h"
#include "vorbis/vorbisfile.h"

#include "./assert.h"

namespace {
	FILE* open_file(const char* file_name) {
		FILE* file = fopen(file_name, "r");
		if (file == nullptr) {
			perror("error opening file");
			todo();
		}
		return file;
	}

	DynArray<i16> read_shorts(const char* file_name) {
		OggVorbis_File vf;

		int err = ov_open(open_file(file_name), &vf, nullptr, 0); // ov_clear will close the file
		check(err == 0);

		uint N_CHANNELS = 2;

		vorbis_info* vi = ov_info(&vf, -1);
		check(vi->rate == 44100);
		check(vi->channels == uint_to_int(N_CHANNELS));

		ogg_int64_t total_samples = ov_pcm_total(&vf, /*-1 means all streams*/ -1);
		DynArray<i16> shorts { safe_mul(i64_to_u32(total_samples), N_CHANNELS) };
		auto* next_out_char = reinterpret_cast<char*>(shorts.begin());
		const char* chars_end = reinterpret_cast<char*>(shorts.end());

		for (;;) {
			constexpr uint word_size = sizeof(i16) / sizeof(char);
			int current_section;
			long ret = ov_read(
				&vf,
				next_out_char,
				long_to_int(chars_end - next_out_char),
				/*bigendian*/ false,
				uint_to_int(word_size),
				/*signed*/ true,
				&current_section);

			if (ret <= 0) {
				if (ret == 0)
					break; // EOF
				todo(); // error in the stream
			} else {
				if (current_section != 0)
					// we don't bother dealing with sample rate changes, etc, but you'll have to
					todo();

				// Note: "ov_read() will decode at most one vorbis packet per invocation, so the value returned will generally be less than length."
				next_out_char += ret;
			}
		}

		ov_clear(&vf); // NOTE: this also closes the file
		check(reinterpret_cast<i16*>(next_out_char) == shorts.slice().end());

		return shorts;
	}

	DecodedAudioFile shorts_to_floats(const DynArray<i16>& shorts) {
		// Now convert to floats
		DecodedAudioFile res { shorts.size() };
		const i16* s = shorts.begin();
		const i16* shorts_end = shorts.end();
		float* f = res.floats.begin();
		constexpr float AMPLITUDE = 1.0f / std::numeric_limits<i16>::max();
		while (s != shorts_end) {
			*f = *s * AMPLITUDE;
			++s;
			++f;
		}
		check(f == res.floats.end());
		return res;
	}
}

DecodedAudioFile read_ogg(const char* file_name) {
	return shorts_to_floats(read_shorts(file_name));
}
