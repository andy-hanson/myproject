#include "./Timer.h"

#include <chrono>
#include <thread>

#include "./util/int.h"
#include "./util/FixedSizeQueue.h"

namespace {
	const std::chrono::duration<double> max_frame_rate = std::chrono::nanoseconds{1000000000} / 30;
}

struct TimerImpl {
	FixedSizeQueue<16, std::chrono::high_resolution_clock::time_point> frame_times;
};

Timer::Timer() : impl{new TimerImpl { FixedSizeQueue<16, std::chrono::high_resolution_clock::time_point> { std::chrono::high_resolution_clock::now() } }} {}
Timer::~Timer() { delete impl;}

// Returns current FPS
double Timer::tick() {
	std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();

	const std::chrono::high_resolution_clock::time_point last_frame_time = impl->frame_times.last_enqueued();
	std::chrono::duration<double> time_since_last_frame = now - last_frame_time;
	while (time_since_last_frame < max_frame_rate) {
		std::chrono::duration<double> diff = max_frame_rate - time_since_last_frame;
		std::this_thread::sleep_for(diff);
		now = std::chrono::high_resolution_clock::now();
		time_since_last_frame = now - last_frame_time;
	}
	//std::cout << (std::chrono::duration_cast<std::chrono::nanoseconds>(time_since_last_frame).count() / 1000000000.0) << std::endl;

	std::chrono::high_resolution_clock::time_point old_frame = impl->frame_times.enqueue_and_dequeue(now);

	long nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(now - old_frame).count();
	double seconds = nanoseconds / 1000000000.0;
	return double(impl->frame_times.size()) / seconds;
}
