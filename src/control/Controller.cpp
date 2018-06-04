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

#include "../util/assert.h"
#include "../util/int.h"

namespace {
	__attribute__((unused))
	const uint MAX_CONTROL_STICKS = 4;

	struct ControlStickInfo {
		i32 minimum;
		i32 maximum;
		i32 fuzz; // margin of error
		i32 flat; // anything less than this should be treated as 0.
	};

	float float_from_joy(const ControlStickInfo& info, float value) {
		return value < info.flat ? 0.0f : value;
	}
}

struct ControllerImpl {
	int fd;
	libevdev* dev;
	std::array<ControlStickInfo, ABS_MAX> sticks_map;

	glm::vec2 joy = glm::vec2(0.0f, 0.0f);
	bool button_is_down = false;
	bool start_is_down = false;

	ControllerGet get() {
		bool button_just_pressed = false;
		bool start_just_pressed = false;

		input_event ev;
		// returns -1 to indicate that no events are available right now -- so sleep.
		int rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
		switch (rc) {
			case libevdev_read_status::LIBEVDEV_READ_STATUS_SUCCESS: {
				//Note: on xbox, ev_abs is the left stick and ev_syn is the right stick.
				// Occationally we will get random tiny events in the other stick, ignore those.
				switch (ev.type) {
					case EV_ABS:
						switch (ev.code) {
							case ABS_X:
							case ABS_Y:
								(ev.code == ABS_X ? joy.x : joy.y) = float_from_joy(sticks_map.at(ev.code), ev.value);
								break;
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
						check(ev.value == 0 || ev.value == 1);
						bool pressed = ev.value == 1;
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
			case -EAGAIN:
				break;
			default:
				unreachable();
		}

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

	std::array<ControlStickInfo, ABS_MAX> sticks_map;
	for (uint i = 0; i < ABS_MAX; ++i)
		if (libevdev_has_event_code(dev, EV_ABS, i)) {
			const input_absinfo* abs_info = libevdev_get_abs_info(dev, i);
			//std::cout << "joystick has absolute axis: " << i << " - " << libevdev_event_code_get_name(EV_ABS, i) << std::endl;
			// fuzz: epxect an error of up to this much
			//std::cout << "  { value: " << abs_info->value << ", min: " << abs_info->minimum << ", max: " << abs_info->maximum
			//		  << ", fuzz: " << abs_info->fuzz << ", flat: " << abs_info->flat << " }" << std::endl;
			sticks_map[i] = ControlStickInfo { abs_info->minimum, abs_info->maximum, abs_info->fuzz, abs_info->flat };
		}
		else
			sticks_map[i] = ControlStickInfo { 0, 0, 0, 0 };

	return Controller { new ControllerImpl { fd, dev, sticks_map } };
}

Controller::~Controller() {
	delete impl;
}

ControllerGet Controller::get() {
	return impl->get();
}
