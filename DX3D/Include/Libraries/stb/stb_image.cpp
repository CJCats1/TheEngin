#include "stb_image.h"
#include <cstdlib>
#include <cstring>
#include <android/log.h>
#if defined(DX3D_PLATFORM_ANDROID)
#include <DX3D/Core/AndroidPlatform.h>
#endif

extern "C" {

// Fallback checkerboard if AImageDecoder not available
static unsigned char* create_checkerboard(int w, int h, int desired_channels) {
    if (w <= 0) w = 256;
    if (h <= 0) h = 256;
    
    int channels = (desired_channels == 0 || desired_channels == 3) ? 3 : 4;
    
    unsigned char* data = (unsigned char*)malloc(w * h * channels);
    if (!data) return nullptr;
    
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int checker = ((x / 8) + (y / 8)) % 2;
            int idx = (y * w + x) * channels;
            
            if (checker) {
                data[idx + 0] = 255;  // R
                data[idx + 1] = 0;    // G
                data[idx + 2] = 255;  // B
            } else {
                data[idx + 0] = 0;    // R
                data[idx + 1] = 0;    // G
                data[idx + 2] = 0;    // B
            }
            
            if (channels == 4) {
                data[idx + 3] = 255;  // A
            }
        }
    }
    
    return data;
}

stbi_uc* stbi_load_from_memory(const stbi_uc* buffer, int len, int* x, int* y,
    int* channels_in_file, int desired_channels)
{
    if (!buffer || len <= 0) {
        if (x) *x = 0;
        if (y) *y = 0;
        if (channels_in_file) *channels_in_file = 0;
        return nullptr;
    }

#if defined(DX3D_PLATFORM_ANDROID)
    // Try to use Android BitmapFactory via JNI
    int width = 0, height = 0, channels = 0;
    unsigned char* pixels = dx3d::platform::decodeImageFromMemory(buffer, len, &width, &height, &channels);
    
    if (pixels && width > 0 && height > 0) {
        __android_log_print(ANDROID_LOG_INFO, "stb_image", "Image decoded via BitmapFactory: %dx%d, channels: %d", width, height, channels);
        
        if (x) *x = width;
        if (y) *y = height;
        if (channels_in_file) *channels_in_file = channels;
        
        // Convert to desired channel format if needed
        if (desired_channels == STBI_rgb || desired_channels == 3) {
            // Convert RGBA to RGB
            unsigned char* rgbPixels = (unsigned char*)malloc(width * height * 3);
            if (rgbPixels) {
                for (int i = 0; i < width * height; i++) {
                    rgbPixels[i * 3 + 0] = pixels[i * 4 + 0];
                    rgbPixels[i * 3 + 1] = pixels[i * 4 + 1];
                    rgbPixels[i * 3 + 2] = pixels[i * 4 + 2];
                }
                free(pixels);
                __android_log_print(ANDROID_LOG_INFO, "stb_image", "Converted to RGB format");
                return rgbPixels;
            }
        }
        
        __android_log_print(ANDROID_LOG_INFO, "stb_image", "Image decoded successfully (RGBA)");
        return pixels;
    } else {
        __android_log_print(ANDROID_LOG_WARN, "stb_image", "BitmapFactory decode failed, falling back to checkerboard");
    }
#endif

    // Fallback: return checkerboard with correct dimensions
    // Try to at least get dimensions from PNG/JPEG headers
    int w = 256, h = 256;
    int fallback_channels = 4;
    
    // Check PNG signature and get dimensions
    if (len >= 24 && buffer[0] == 137 && buffer[1] == 80 && buffer[2] == 78 && buffer[3] == 71) {
        w = ((int)buffer[16] << 24) | ((int)buffer[17] << 16) | ((int)buffer[18] << 8) | buffer[19];
        h = ((int)buffer[20] << 24) | ((int)buffer[21] << 16) | ((int)buffer[22] << 8) | buffer[23];
        __android_log_print(ANDROID_LOG_INFO, "stb_image", "PNG detected: %dx%d (fallback)", w, h);
        fallback_channels = 4;
    }
    // Check JPEG signature and find SOF marker for dimensions
    else if (len >= 4 && buffer[0] == 0xFF && buffer[1] == 0xD8) {
        __android_log_print(ANDROID_LOG_INFO, "stb_image", "JPEG detected, searching for dimensions...");
        fallback_channels = 3;
        for (int pos = 2; pos + 8 < len; pos++) {
            if (buffer[pos] == 0xFF && (buffer[pos+1] == 0xC0 || buffer[pos+1] == 0xC1)) {
                if (pos + 9 < len) {
                    h = ((int)buffer[pos + 5] << 8) | buffer[pos + 6];
                    w = ((int)buffer[pos + 7] << 8) | buffer[pos + 8];
                    __android_log_print(ANDROID_LOG_INFO, "stb_image", "JPEG dimensions: %dx%d (fallback)", w, h);
                    break;
                }
            }
        }
    }
    
    if (x) *x = w;
    if (y) *y = h;
    if (channels_in_file) *channels_in_file = fallback_channels;
    
    __android_log_print(ANDROID_LOG_WARN, "stb_image", "Using checkerboard fallback");
    return create_checkerboard(w, h, desired_channels);
}

stbi_uc* stbi_load_from_callbacks(const stbi_io_callbacks* clbk, void* user,
    int* x, int* y, int* channels_in_file, int desired_channels)
{
    (void)clbk;
    (void)user;
    (void)x;
    (void)y;
    (void)channels_in_file;
    (void)desired_channels;
    return nullptr;
}

void stbi_image_free(void* retval_from_stbi_load)
{
    if (retval_from_stbi_load) {
        free(retval_from_stbi_load);
    }
}

}
