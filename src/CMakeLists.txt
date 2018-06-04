cmake_minimum_required(VERSION 3.10)
project(myproject)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_COMPILER "/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++")

# -stdlib=libstdc++ supposedly improves debugging (https://blog.jetbrains.com/clion/2015/05/debug-clion/)
# Without -D_GLIBCXX_DEBUG I can't inspect strings
set(CMAKE_CXX_FLAGS "-pedantic -Weverything -Wno-c++98-compat-pedantic -Wno-padded -Wno-missing-noreturn -Werror -stdlib=libstdc++ -D_GLIBCXX_DEBUG")

find_package(OpenGL 4.5 REQUIRED)

find_package(glfw3 REQUIRED)

find_package(GLEW REQUIRED)

find_package(PNG REQUIRED)
include_directories(${PNG_INCLUDE_DIR})

find_package(Threads REQUIRED)

#include_directories(/usr/include/ois)
include_directories(/usr/include/libevdev-1.0/libevdev)
include_directories(GLFW3_INCLUDE_DIRS)
include_directories(GLEW_INCLUDE_DIRS)
link_libraries(${GLEW_LIBRARIES})
link_libraries(${OPENGL_LIBRARY})

add_executable(myproject
	./main.cpp

	./audio/audio.cpp
	./audio/audio.h
	./audio/audio_file.h
	./audio/read_wav.h
	./audio/read_wav.cpp
	./audio/read_ogg.h
	./audio/read_ogg.cpp

	./control/Controller.h
	./control/Controller.cpp

	./graphics/model.h
	./graphics/process_model.cpp
	./graphics/process_model.h
	./graphics/shaders.h
	./graphics/shaders.cpp
	./graphics/read_obj.h
	./graphics/read_obj.cpp
	./graphics/read_png.cpp
	./graphics/read_png.h
	./graphics/RenderableModel.h

	./util/assert.h
	./util/DynArray.h
	./util/float.h
	./util/int.h
	./util/io.cpp
	./util/io.h
	./util/math.h
	./util/Matrix.h
	./util/Slice.h

	./vendor/readerwriterqueue/atomicops.h
	./vendor/readerwriterqueue/readerwriterqueue.h)
target_link_libraries(myproject glfw evdev OIS soundio sndfile vorbisfile Threads::Threads ${PNG_LIBRARY})
