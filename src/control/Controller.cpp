#include "./Controller.h"

#include <fcntl.h> // open
#include <iostream> // std::cerr
#include <unistd.h>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation-unknown-command"
#pragma clang diagnostic ignored "-Wdocumentation-deprecated-sync"
#include <libevdev.h>
#pragma clang diagnostic pop

#include <string.h> // strerror
#include <thread>
#include <glm/geometric.hpp> // length
#include <glm/gtx/norm.hpp>

#include "../util/assert.h"
#include "../util/int.h"

namespace {
	struct AxisInfo {
		i32 minimum;
		i32 maximum;
		i32 fuzz; // margin of error
		i32 flat; // anything less than this should be treated as 0.
	};

	//TODO: on my controller 'flat' seems to need to be bigger. But don't hard-code!
	const int FLAT_FACTOR = 4;
	int int_from_joy(int current, const AxisInfo& info, int new_value) {
		if (int_abs(new_value) < info.flat * FLAT_FACTOR) return 0;
		int diff = int_abs(new_value - current);
		return diff < info.fuzz ? current : new_value;
	}

	float float_from_joy(int value, const AxisInfo& info) {
		return value < 0 ? -(float(value) / float(info.minimum)) : float(value) / float(info.maximum);
	}
}

struct ControllerImpl {
	int fd;
	libevdev* dev;
	std::array<AxisInfo, ABS_MAX> sticks_map;

	int joy_x;
	int joy_y;
	glm::vec2 joy = glm::vec2(0.0f, 0.0f);
	bool button_is_down = false;
	bool start_is_down = false;

	ControllerGet get() {
		bool button_just_pressed = false;
		bool start_just_pressed = false;

		input_event ev;

		for (;;) {
			// returns -EAGAIN to indicate that no events are available right now.
			int rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
			switch (rc) {
				case libevdev_read_status::LIBEVDEV_READ_STATUS_SUCCESS: {
					//Note: on xbox, ev_abs is the left stick and ev_syn is the right stick.
					// Occationally we will get random tiny events in the other stick, ignore those.
					switch (ev.type) {
						case EV_ABS:
							switch (ev.code) {
								case ABS_X:
								case ABS_Y: {
									int& i = ev.code == ABS_X ? joy_x : joy_y;
									float& f = (ev.code == ABS_X ? joy.x : joy.y);
									const AxisInfo& info = sticks_map.at(ev.code);
									i = int_from_joy(i, info, ev.value);
									f = float_from_joy(i, info);
									if (ev.code == ABS_Y) f = -f; // It's upside-down for some reason.
									break;
								}
								case ABS_RX:
								case ABS_RY:
								case ABS_Z: // ZL
								case ABS_RZ: // ZR
									break; // ignore
								default:
									std::cerr << libevdev_event_code_get_name(ev.type, ev.code) << std::endl;
									todo();
							}
							break;
						case EV_SYN:
							// think I can ignore these:
							break;
						case EV_KEY: {
							bool pressed = int_to_bool(ev.value);
							switch (ev.code) {
								case BTN_NORTH:
								case BTN_WEST:
								case BTN_EAST:
								case BTN_SOUTH:
									assert(ev.value == 0 || ev.value == 1);
									button_just_pressed = !button_is_down;
									button_is_down = pressed;
									break;

								case BTN_TL:
								case BTN_TR:
								case ABS_HAT0X:
								case ABS_HAT0Y:
									break; // ignore

								case BTN_SELECT:
								case BTN_START:
									start_just_pressed = !start_is_down;
									start_is_down = pressed;
									break;

								default:
									std::cerr << libevdev_event_code_get_name(ev.type, ev.code) << std::endl;
									todo();
							}
							break;
						}

						default:
							std::cerr << libevdev_event_type_get_name(ev.type) << std::endl;
							todo();
					}

					break;
				}
				case libevdev_read_status::LIBEVDEV_READ_STATUS_SYNC:
					//todo();
				case -EAGAIN: // Means: no more events are available.
					goto done;
				default:
					unreachable();
			}
		}

		done:
		float len = glm::length2(joy);
		check(len <= 2.0f);
		if (len > 1.0f)
			joy = glm::normalize(joy);
		return ControllerGet { joy, button_is_down, button_just_pressed, start_just_pressed };
	}

	~ControllerImpl() {
		close(fd);
		//TODO: close dev?
	}
};

Controller Controller::start() {
	// See https://gist.github.com/meghprkh/9cdce0cd4e0f41ce93413b250a207a55 for example of being smarter about this
	int fd = open("/dev/input/event12", O_RDONLY|O_NONBLOCK); //TODO: probably not portable, but this is the xbox controller for me
	libevdev *dev;

	int err = libevdev_new_from_fd(fd, &dev);
	if (err != 0) {
		std::cerr << "libevdev failure: " << strerror(-err) << std::endl;
		todo();
	}
	//std::cout << "Input device name: \"" << libevdev_get_name(dev) << "\"" << std::endl;

	std::array<AxisInfo, ABS_MAX> sticks_map;
	for (uint i = 0; i < ABS_MAX; ++i)
		if (libevdev_has_event_code(dev, EV_ABS, i)) {
			const input_absinfo* abs_info = libevdev_get_abs_info(dev, i);
			//std::cout << "joystick has absolute axis: " << i << " - " << libevdev_event_code_get_name(EV_ABS, i) << std::endl;
			// fuzz: epxect an error of up to this much
			//std::cout << "  { value: " << abs_info->value << ", min: " << abs_info->minimum << ", max: " << abs_info->maximum
			//		  << ", fuzz: " << abs_info->fuzz << ", flat: " << abs_info->flat << " }" << std::endl;
			sticks_map[i] = AxisInfo { abs_info->minimum, abs_info->maximum, abs_info->fuzz, abs_info->flat };
		}
		else
			sticks_map[i] = AxisInfo { 0, 0, 0, 0 };

	return { new ControllerImpl { fd, dev, sticks_map, /*joy_x*/ 0, /*joy_y*/ 0, /*joy*/ glm::vec2(0.0) } };
}

Controller::~Controller() {
	delete impl;
}

ControllerGet Controller::get() {
	return impl->get();
}
