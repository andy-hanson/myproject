#pragma once

#include <new>
#include <vector>
#include "./int.h"

#include "./Slice.h"

template <typename T>
class MutableSlice {
	T* _begin;
	u32 _size;

public:
	inline MutableSlice() : _begin{nullptr}, _size{0} {}
	inline MutableSlice(T* begin, T* end) : _begin{begin}, _size{i64_to_u32(end - begin)} {}

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

/** Basically just a Slice with a destructor. */
template <typename T>
class DynArray {
	MutableSlice<T> _slice;

	DynArray(T* begin, u32 len) : _slice{begin, begin + len} {}

public:
	DynArray(u32 len) : DynArray{new T[len], len} {}
	DynArray(const DynArray& other __attribute__((unused))) : _slice{} {
		todo(); // should be optimized away!
	}
	DynArray(DynArray&& other) {
		_slice = other._slice;
		other._slice = MutableSlice<T> {};
	}
	~DynArray() {
		::operator delete(_slice.begin());
	}

	static DynArray copy_slice(const Slice<T>& slice) {
		T* begin = static_cast<T*>(::operator new(sizeof(T) * slice.size()));
		for (u32 i = 0; i < slice.size(); ++i)
			begin[i] = slice[i];
		return { begin, slice.size() };
	}

	MutableSlice<T> slice() { return _slice; }

	//TODO: kill and make them cast to slice?
	inline T& operator[](u32 i) {
		return _slice[i];
	}
	inline const T& operator[](u32 i) const {
		return _slice[i];
	}

	inline const T* begin() const { return _slice.begin(); }
	inline const T* end() const { return _slice.end(); }
	inline T* begin() { return _slice.begin(); }
	inline T* end() { return _slice.end(); }
	inline u32 size() const { return _slice.size(); }
};

//TODO:MOVE
template <typename Out>
struct map {
	template <typename In, typename /*const In& => Out*/ Cb>
	DynArray<Out> operator()(const Slice<In>& slice, Cb cb) {
		DynArray<Out> out { slice.size() };
		for (u32 i = 0; i != slice.size(); ++i)
			out[i] = cb(slice[i]);
		return out;
	}
};

//TODO:MOVE
template <typename T>
inline Slice<T> vec_to_slice(std::vector<T>& v) {
	T* begin = &v[0];
	T* end = begin + v.size();
	return { begin, end };
}

//TODO:MOVE
template <typename T>
u8 index_of(const Slice<T>& slice, const T& value) {
	u8 size = u32_to_u8(slice.size());
	for (u8 i = 0; i != size; ++i) {
		if (slice[i] == value)
			return i;
	}
	unreachable();
}

//TODO:MOVE
template <typename T>
inline DynArray<T> vec_to_dyn_array(std::vector<T>& v) {
	return DynArray<T>::copy_slice(vec_to_slice(v));
}
