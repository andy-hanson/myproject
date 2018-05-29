cmake_minimum_required(VERSION 3.10)
project(myproject)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_COMPILER "/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++")

# -stdlib=libstdc++ supposedly improves debugging (https://blog.jetbrains.com/clion/2015/05/debug-clion/)
# Without -D_GLIBCXX_DEBUG I can't inspect strings
set(CMAKE_CXX_FLAGS "-pedantic -Weverything -Wno-c++98-compat-pedantic -Wno-padded -Werror -stdlib=libstdc++ -D_GLIBCXX_DEBUG")

find_package(OpenGL 4.5 REQUIRED)

find_package(glfw3 REQUIRED)

find_package(GLEW REQUIRED)

find_package(PNG REQUIRED)
include_directories(${PNG_INCLUDE_DIR})

#find_package(GLUT REQUIRED)

#find_package(SDL2 REQUIRED)
#include_directories(${SDL2_INCLUDE_DIRS})

include_directories(GLFW3_INCLUDE_DIRS)
include_directories(GLEW_INCLUDE_DIRS)
link_libraries(${GLEW_LIBRARIES})
link_libraries(${OPENGL_LIBRARY})
#link_libraries(${GLUT_LIBRARIES})

add_executable(myproject
	main.cpp
	util/Matrix.h
	util/read_png.h
	util/int.h
	util/DynArray.h
	util/read_png.cpp
	util/io.h
	util/io.cpp util/read_obj.h util/read_obj.cpp process_model.cpp process_model.h model.h)
target_link_libraries(myproject glfw ${PNG_LIBRARY})
