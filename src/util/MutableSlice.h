#pragma once

#include "./int.h"
#include "./Slice.h"

template <typename T>
class MutableSlice {
	T* _begin;
	u32 _size;

public:
	inline MutableSlice() : _begin{nullptr}, _size{0} {}
	inline MutableSlice(T* begin, u32 size) : _begin{begin}, _size{size} {}

	inline u32 size() const {
		return _size;
	}

	using const_iterator = const T*;
	inline const_iterator begin() const { return _begin; }
	inline const_iterator end() const { return _begin + _size; }
	using iterator = T*;
	inline iterator begin() { return _begin; }
	inline iterator end() { return _begin + _size; }

	inline T& operator[](u32 i) {
		check(i < size());
		return *(_begin + i);
	}
	inline const T& operator[](u32 i) const {
		check(i < size());
		return *(_begin + i);
	}

	inline operator Slice<T>() const { return Slice<T> { _begin, _size }; }
};
