/* stb_image - v2.30 - public domain image loader
https://github.com/nothings/stb

This is a minimalist implementation supporting PNG, JPEG, and basic formats
*/

#ifndef STB_IMAGE_H
#define STB_IMAGE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char stbi_uc;
typedef unsigned short stbi_us;

typedef struct {
    int (*read)(void* user, char* data, int size);
    void (*skip)(void* user, int n);
    int (*eof)(void* user);
} stbi_io_callbacks;

// Flags for desired_channels
#define STBI_default 0
#define STBI_grey 1
#define STBI_grey_alpha 2
#define STBI_rgb 3
#define STBI_rgb_alpha 4

stbi_uc* stbi_load_from_memory(const stbi_uc* buffer, int len, int* x, int* y,
    int* channels_in_file, int desired_channels);
stbi_uc* stbi_load_from_callbacks(const stbi_io_callbacks* clbk, void* user,
    int* x, int* y, int* channels_in_file, int desired_channels);
void stbi_image_free(void* retval_from_stbi_load);

#ifdef __cplusplus
}
#endif

#endif // STB_IMAGE_H
