#pragma once

#include <new>
#include <vector>

#include "./MutableSlice.h"

/** Basically just a Slice with a destructor. */
template <typename T>
class DynArray {
	MutableSlice<T> _slice;

	DynArray(T* begin, u32 size) : _slice{begin, size} {}

	// uninitialized
	DynArray(u32 size) : DynArray{static_cast<T*>(::operator new(sizeof(T) *  size)), size} {
		check(size != 0);
	}

public:
	DynArray() : _slice{} {}
	inline static DynArray<T> uninitialized(u32 len) { return DynArray { len }; }
	//DynArray(const DynArray& other __attribute__((unused))) : _slice{} {
	//	todo(); // should be optimized away!
	//}
	void operator=(DynArray&& other) {
		::operator delete(_slice.begin());
		_slice = other._slice;
		other._slice = MutableSlice<T> {};
	}
	DynArray(DynArray&& other) : _slice{other._slice} {
		other._slice = MutableSlice<T> {};
	}
	~DynArray() {
		::operator delete(_slice.begin());
	}

	//TODO:KILL?
	static DynArray<T> copy_slice(const Slice<T>& slice) {
		DynArray<T> ret = uninitialized(slice.size());
		for (u32 i = 0; i < slice.size(); ++i)
			ret[i] = slice[i];
		return ret;
	}

	MutableSlice<T> mutable_slice() { return _slice; }
	Slice<T> slice() const { return _slice; }

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

template <typename T>
struct fill_array {
	template <typename Cb>
	DynArray<T> operator()(u32 size, Cb cb) {
		DynArray<T> out = DynArray<T>::uninitialized(size);
		for (u32 i = 0; i != size; ++i)
			new (&out[i]) T { cb(i) };
		return out;
	}
};

//TODO:MOVE
template <typename Out>
struct map {
	template <typename In, typename /*const In& => Out*/ Cb>
	DynArray<Out> operator()(const Slice<In>& slice, Cb cb) {
		DynArray<Out> out = DynArray<Out>::uninitialized(slice.size());
		for (u32 i = 0; i != slice.size(); ++i)
			new (&out[i]) Out { cb(slice[i]) };
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
