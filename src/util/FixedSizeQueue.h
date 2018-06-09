#pragma once

#include "./int.h"

//TODO:MOVE
template <u32 _size, typename T>
class FixedSizeQueue {
	T values[_size];
	u32 index; // This points to the *next* element to enqueue (which is also the next to dequeue!)

public:
	FixedSizeQueue(T init) : index{0} {
		for (T& v : values) {
			v = init;
		}
	}

	inline u32 size() const { return _size; }

	const T& last_enqueued() {
		return values[index == 0 ? _size - 1 : index - 1];
	}

	T enqueue_and_dequeue(T new_value) {
		static_assert(_size != 0);
		T ret = values[index];
		values[index] = new_value;
		++index;
		if (index == _size) index = 0;
		return ret;
	}
};
