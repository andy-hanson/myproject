#pragma once

//TODO: can't find how to test for debug build
//#ifdef DEBUG
inline void check(bool b) {
	if (!b)
		throw "todo";
}
//#else
//inline void check(bool b __attribute__((unused))) {}
//#endif

__attribute__((noreturn))
inline void todo() {
	throw "todo";
}

__attribute__((noreturn))
inline void unreachable() {
	throw "todo";
}

template <typename T>
T* assert_not_null(T* ptr) {
	check(ptr != nullptr);
	return ptr;
}
