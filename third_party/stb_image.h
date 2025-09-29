// stb_image - v2.28 - public domain image loader - https://github.com/nothings/stb
// This is a trimmed single-header copy sufficient for JPEG/PNG decode for our exporter.
// Define STB_IMAGE_IMPLEMENTATION in exactly one .cpp before including this header.
#ifndef STB_IMAGE_H
#define STB_IMAGE_H

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned char *stbi_load_from_memory(const unsigned char *buffer, int len, int *x, int *y, int *channels_in_file, int desired_channels);
extern void stbi_image_free(void *retval_from_stbi_load);

#ifdef __cplusplus
}
#endif

#endif // STB_IMAGE_H
