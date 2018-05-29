#pragma once

#include <new>
#include "./int.h"

//TODO:MOVE
template <typename T>
class Slice {
	T* _begin;
	u32 _size;

public:
	inline Slice(T* begin, T* end) : _begin{begin}, _size{i64_to_u32(end - begin)} {}
	Slice(const Slice& other) = delete; // Don't want to lose const-ness

	inline u32 size() const {
		return _size;
	}

	inline T& operator[](u32 i) {
		assert(i < size());
		return *(_begin + i);
	}
	inline const T& operator[](u32 i) const {
		assert(i < size());
		return *(_begin + i);
	}

	using const_iterator = const T*;
	inline const_iterator begin() const { return _begin; }
	inline const_iterator end() const { return _begin + _size; }
	using iterator = T*;
	inline iterator begin() { return _begin; }
	inline iterator end() { return _begin + _size; }
};

/** Basically just a Slice with a destructor. */
template <typename T>
class DynArray {
	Slice<T> _slice;

	DynArray(T* begin, u32 len) : _slice{begin, begin + len} {}

public:
	DynArray(u32 len) : DynArray{new T[len], len} {}
	DynArray(const DynArray& other) = delete;
	DynArray(DynArray&& other) = default;
	~DynArray() {
		::operator delete(_slice.begin());
	}

	static DynArray copy_slice(const Slice<T>& slice) {
		T* begin = static_cast<T*>(::operator new(sizeof(T) * slice.size()));
		for (u32 i = 0; i < slice.size(); ++i)
			begin[i] = slice[i];
		return { begin, slice.size() };
	}

	Slice<T>& slice() { return _slice; }
	const Slice<T>& slice() const { return _slice; }

	//TODO: kill and make them cast to slice?
	inline T& operator[](u32 i) {
		return _slice[i];
	}
	inline const T& operator[](u32 i) const {
		return _slice[i];
	}

	inline typename Slice<T>::const_iterator begin() const { return _slice.begin(); }
	inline typename Slice<T>::const_iterator end() const { return _slice.end(); }
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
