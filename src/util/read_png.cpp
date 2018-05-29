#include "./read_png.h"

#define PNG_DEBUG 3
#include <png.h>
#include "./DynArray.h"
#include "./int.h"

Matrix<uint32_t> png_texture_load(const char* file_name) {
	png_byte header[8];

	FILE *fp = fopen(file_name, "rb");
	assert(fp);

	const uint HEADER_BYTES = 8;

	fread(header, 1, HEADER_BYTES, fp);

	assert(!png_sig_cmp(header, 0, HEADER_BYTES));

	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	assert(png_ptr);
	png_infop info_ptr = png_create_info_struct(png_ptr);
	png_infop end_info = png_create_info_struct(png_ptr);
	assert(info_ptr && end_info);

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
	assert(rowbytes % 4 == 0); // opengl requires this
	assert(rowbytes == width * 4); // r, g, b, a
	static_assert(sizeof(uint32_t) == sizeof(png_byte) * 4);

	Matrix<uint32_t> image_data { width, height };

	DynArray<png_bytep> row_pointers { height };
	for (uint y = 0; y < height; y++)
		row_pointers[y] = static_cast<png_byte*>(static_cast<void*>(image_data.row_pointer(y)));

	png_read_image(png_ptr, row_pointers.slice().begin());

	// clean up
	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
	fclose(fp);

	return image_data;
}
