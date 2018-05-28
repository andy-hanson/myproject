#pragma once

#include "./int.h"

//TODO:NEATER
template <typename T>
class DynArray {
	T* _begin;
	uint32_t _size;

public:
	DynArray(uint32_t len) : _size{len} {
		_begin = new T[_size];
	}

	~DynArray() {
		delete[] _begin;
	}

	inline T& operator[](uint32_t i) {
		assert(i < _size);
		return *(_begin + i);
	}
	inline const T& operator[](uint32_t i) const {
		assert(i < _size);
		return *(_begin + i);
	}

	inline T* begin() {
		return _begin;
	}
};
