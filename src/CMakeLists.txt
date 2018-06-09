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

include_directories(/usr/include/libevdev-1.0/libevdev)
include_directories(GLFW3_INCLUDE_DIRS)
include_directories(GLEW_INCLUDE_DIRS)
link_libraries(${GLEW_LIBRARIES})
link_libraries(${OPENGL_LIBRARY})

include_directories(../vendor/reactphysics3d/src)

add_executable(myproject
	./game.cpp
	./game.h
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

	./graphics/convert_model.cpp
	./graphics/convert_model.h
	./graphics/gl_types.h
	./graphics/Graphics.h
	./graphics/Graphics.cpp
	./graphics/read_png.cpp
	./graphics/read_png.h
	./graphics/RenderableModel.h
	./graphics/shader_utils.h
	./graphics/shader_utils.cpp

	./model/Color.h
	./model/Model.h
	./model/ModelKind.h
	./model/parse_model.cpp
	./model/parse_model.h

	./physics/Physics.h
	./physics/Physics.cpp

	./util/assert.h
	./util/DynArray.h
	./util/FixedArray.h
	./util/float.h
	./util/int.h
	./util/io.cpp
	./util/io.h
	./util/math.h
	./util/Matrix.h
	./util/MutableSlice.h
	./util/Quaternion.h
	./util/Ref.h
	./util/Slice.h
	./util/string.h
	./util/Transform.h
	./util/UniquePtr.h

	./vendor/readerwriterqueue/atomicops.h
	./vendor/readerwriterqueue/readerwriterqueue.h

	./Timer.cpp
	./Timer.h util/FixedSizeQueue.h)
target_link_libraries(myproject glfw evdev OIS reactphysics3d soundio sndfile vorbisfile Threads::Threads ${PNG_LIBRARY})
