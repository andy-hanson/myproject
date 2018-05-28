#pragma once

#include <cassert>
#include <cstdint>

struct Coord {
	uint32_t x;
	uint32_t y;
};

// In this matrix, *rows* are stored contiguously.
template <typename T>
class Matrix {
	uint32_t _width;
	uint32_t _height;
	T* _data;

public:
	Matrix(uint32_t width, uint32_t height) : _width{width}, _height{height}, _data{new T[width * height]} {}
	~Matrix() {
		delete[] _data;
	}
	Matrix(Matrix&& other) = default;
	Matrix(const Matrix& other) = delete;

	inline uint32_t width() const { return _width; }
	inline uint32_t height() const { return _height; }

	inline T& operator[](Coord c) {
		assert(c.x < _width && c.y < _height);
		return _data[c.x + c.y * _width];
	}
	inline const T& operator[](Coord c) const {
		assert(c.x < _width && c.y < _height);
		return _data[c.x + c.y * _width];
	}

	inline const T* raw() const {
		return _data;
	}

	inline T* row_pointer(uint32_t y) {
		assert(y < _height);
		return _data + y * _width;
	}
};
