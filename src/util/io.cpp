#include "./io.h"

#include <fstream>
#include <sstream>
#include <unistd.h> // getcwd

#include "./assert.h"

namespace {
	char* get_end(char* begin) {
		char* c = begin;
		while (*c != '\0')
			++c;
		return c;
	}

	const char* strip_last_part(const char* begin, const char* end) {
		while (end != begin && *end != '/')
			--end;
		return end;
	}
}

std::string get_current_directory() {
	const uint MAX_SIZE = 100;
	char buf[MAX_SIZE];
	if (!getcwd(buf, MAX_SIZE)) todo();

	// Strip out '/src/cmake-build-debug'
	const char* begin = buf;
	const char* cc = strip_last_part(buf, get_end(buf));
	check(cc > buf && *cc == '/');
	--cc;
	cc = strip_last_part(buf, cc);
	return std::string { begin, cc };
}

std::string read_file(const std::string& file_name) {
	std::ifstream i { file_name };
	check(bool(i));
	std::stringstream buffer;
	buffer << i.rdbuf();
	return buffer.str();
}
