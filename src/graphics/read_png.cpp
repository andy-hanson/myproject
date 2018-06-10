#include "./read_png.h"

#define PNG_DEBUG 3 // TODO: #ifdef DEBUG
#include <png.h>
#include "./assert.h"
#include "../util/DynArray.h"
#include "../util/int.h"

namespace {
	const uint DEPTH = 8;
	const uint PIXEL_SIZE = 3;

	struct pixel { u8 r; u8 g; u8 b; };

	pixel pixel_at(Slice<u8> bitmap, uint width, uint height, uint x, uint y) {
		check(x < width && y < height);
		uint i = y * width + x;
		check(i < width * height);
		return pixel { bitmap[i * 3], bitmap[i * 3 + 1], bitmap[i * 3 + 2] };
	}
}

void write_png(u32 width, u32 height, Slice<u8> bitmap, const char* file_name) {
	png_structp png_ptr = assert_not_null(png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr));

	png_infop info_ptr = assert_not_null(png_create_info_struct(png_ptr));

	png_set_IHDR(png_ptr, info_ptr, width, height, DEPTH, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	//TODO: avoid malloc
	png_byte** row_pointers = static_cast<png_byte**>(png_malloc(png_ptr, height * sizeof(png_byte*)));
	for (uint y = 0; y != height; ++y) {
		png_byte* row = static_cast<png_byte*>(png_malloc(png_ptr, sizeof(uint8_t) * width * PIXEL_SIZE));
		row_pointers[y] = row;
		//TODO: this whole loop is just memcpy?
		for (uint x = 0; x != width; ++x) {
			pixel pixel = pixel_at(bitmap, width, height, x, y);
			*row++ = pixel.r;
			*row++ = pixel.g;
			*row++ = pixel.b;
		}
	}

	FILE* fp = assert_not_null(fopen(file_name, "wb"));
	png_init_io(png_ptr, fp);
	png_set_rows(png_ptr, info_ptr, row_pointers);
	png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, nullptr);

	for (size_t y = 0; y != height; ++y) {
		png_free(png_ptr, row_pointers[y]);
	}
	png_free(png_ptr, row_pointers);

	png_destroy_write_struct(&png_ptr, &info_ptr);
}

Matrix<uint32_t> png_texture_load(const char* file_name) {
	png_byte header[8];

	FILE *fp = assert_not_null(fopen(file_name, "rb"));

	const uint HEADER_BYTES = 8;

	fread(header, 1, HEADER_BYTES, fp);

	check(!png_sig_cmp(header, 0, HEADER_BYTES));

	png_structp png_ptr = assert_not_null(png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr));
	png_infop info_ptr = assert_not_null(png_create_info_struct(png_ptr));
	png_infop end_info = assert_not_null(png_create_info_struct(png_ptr));

	// init png reading
	png_init_io(png_ptr, fp);
	// let libpng know you already read the first 8 bytes
	png_set_sig_bytes(png_ptr, HEADER_BYTES);
	// read all the info up to the image data
	png_read_info(png_ptr, info_ptr);

	int bit_depth, color_type;
	png_uint_32 width_64, height_64; // NOTE: This is actually a uint64_t, because libpng sucks at naming things
	png_get_IHDR(png_ptr, info_ptr, &width_64, &height_64, &bit_depth, &color_type, nullptr, nullptr, nullptr);
	uint32_t width = u64_to_u32(width_64);
	uint32_t height = u64_to_u32(height_64);

	// Update the png info struct.
	png_read_update_info(png_ptr, info_ptr);

	// Row size in bytes. glTexImage2d requires rows to be 4-byte aligned.
	uint32_t rowbytes = u64_to_u32(png_get_rowbytes(png_ptr, info_ptr));
	check(rowbytes % 4 == 0); // opengl requires this
	check(rowbytes == width * 4); // r, g, b, a
	static_assert(sizeof(uint32_t) == sizeof(png_byte) * 4);

	Matrix<uint32_t> image_data { width, height };

	DynArray<png_bytep> row_pointers = DynArray<png_bytep>::uninitialized(height);
	for (uint y = 0; y < height; y++)
		row_pointers[y] = static_cast<png_byte*>(static_cast<void*>(image_data.row_pointer(y)));

	png_read_image(png_ptr, row_pointers.mutable_slice().begin());

	// clean up
	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
	fclose(fp);

	return image_data;
}
