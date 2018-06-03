#pragma once

#include "./Slice.h"
#include "./Matrix.h"

Matrix<uint32_t> png_texture_load(const char* file_name);
void write_png(u32 width, u32 height, Slice<u8> bitmap, const char* file_name);
