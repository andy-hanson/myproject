#pragma once

#include "./assert.h"
#include "./int.h"

struct Coord {
	u32 x;
	u32 y;
};

// In this matrix, *rows* are stored contiguously.
template <typename T>
class Matrix {
	u32 _width;
	u32 _height;
	T* _data;

public:
	Matrix(uint32_t width, uint32_t height) : _width{width}, _height{height}, _data{new T[width * height]} {}
	~Matrix() {
		delete[] _data;
	}
	Matrix(Matrix&& other) = default;
	Matrix(const Matrix& other) = delete;

	inline u32 width() const { return _width; }
	inline u32 height() const { return _height; }

	inline T& operator[](Coord c) {
		check(c.x < _width && c.y < _height);
		return _data[c.x + c.y * _width];
	}
	inline const T& operator[](Coord c) const {
		check(c.x < _width && c.y < _height);
		return _data[c.x + c.y * _width];
	}

	inline const T* raw() const {
		return _data;
	}

	inline T* row_pointer(uint32_t y) {
		check(y < _height);
		return _data + y * _width;
	}
};
