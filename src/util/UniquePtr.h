#pragma once

template <typename T>
class UniquePtr {
	T* _ptr;

public:
	UniquePtr(const UniquePtr& other) = delete;
	inline UniquePtr() : _ptr{nullptr} {}
	inline explicit UniquePtr(T* ptr) : _ptr{ptr} {}

	inline UniquePtr(UniquePtr<T>&& other) : _ptr(other._ptr) {
		other._ptr = nullptr;
	}
	inline void operator=(UniquePtr<T>&& other) {
		assert(_ptr == nullptr);
		_ptr = other._ptr;
		other._ptr = nullptr;
	}

	~UniquePtr() {
		delete _ptr;
	}

	inline T* ptr() {
		return _ptr;
	}

	inline T* operator->() {
		check(_ptr != nullptr);
		return _ptr;
	}
};
