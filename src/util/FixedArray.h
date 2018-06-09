#pragma once

#include "./int.h"
#include "./MutableSlice.h"
#include "./Slice.h"

template <u32 size, typename T>
class FixedArray {
	T data[size];

public:
	MutableSlice<T> mutable_slice() {
		return { data, size };
	}

	Slice<T> slice() const {
		return { data, size };
	}

	T& operator[](u32 i) {
		return mutable_slice()[i];
	}
};
