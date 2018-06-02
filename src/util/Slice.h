#pragma once

#include "./int.h"

template <typename T>
class Slice {
	const T* _begin;
	u32 _size;

public:
	inline Slice() : _begin{nullptr}, _size{0} {}
	inline Slice(const T* begin, u32 size) : _begin{begin}, _size{size} {}
	inline Slice(const T* begin, const T* end) : _begin{begin}, _size{i64_to_u32(end - begin)} {}

	inline u32 size() const {
		return _size;
	}

	inline const T& operator[](u32 i) const {
		check(i < size());
		return *(_begin + i);
	}

	using const_iterator = const T*;
	inline const_iterator begin() const { return _begin; }
	inline const_iterator end() const { return _begin + _size; }

	inline bool is_empty() { return _size == 0; }
};
