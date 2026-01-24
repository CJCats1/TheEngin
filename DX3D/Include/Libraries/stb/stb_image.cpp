#include "stb_image.h"
#include <cstdlib>
#include <cstring>
#include <android/log.h>
#include <android/api-level.h>

// Try to include AImageDecoder if available
#if __ANDROID_API__ >= 29
#include <android/imagedecoder.h>
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

#if defined(__ANDROID_API__) && __ANDROID_API__ >= 30
    __android_log_print(ANDROID_LOG_INFO, "LoadTexture2D", "Attempting AImageDecoder");
    
    AImageDecoder* decoder = nullptr;
    int result = AImageDecoder_createFromBuffer(buffer, len, &decoder);
    
    if (result == ANDROID_IMAGE_DECODER_SUCCESS && decoder) {
        const AImageDecoderHeaderInfo* info = AImageDecoder_getHeaderInfo(decoder);
        int width = AImageDecoderHeaderInfo_getWidth(info);
        int height = AImageDecoderHeaderInfo_getHeight(info);
        
        __android_log_print(ANDROID_LOG_INFO, "LoadTexture2D", "Decoded: %dx%d", width, height);
        
        if (x) *x = width;
        if (y) *y = height;
        
        // Set format to RGBA_8888
        AImageDecoder_setAndroidBitmapFormat(decoder, ANDROID_BITMAP_FORMAT_RGBA_8888);
        if (channels_in_file) *channels_in_file = 4;
        
        // Decode
        size_t stride = AImageDecoder_getMinimumStride(decoder);
        unsigned char* pixelBuffer = (unsigned char*)malloc(stride * height);
        
        if (pixelBuffer) {
            result = AImageDecoder_decode(decoder, pixelBuffer, stride);
            AImageDecoder_delete(decoder);
            
            if (result == ANDROID_IMAGE_DECODER_SUCCESS) {
                __android_log_print(ANDROID_LOG_INFO, "LoadTexture2D", "Success!");
                return pixelBuffer;
            }
            free(pixelBuffer);
        } else {
            AImageDecoder_delete(decoder);
        }
    } else if (decoder) {
        AImageDecoder_delete(decoder);
    }
    
    __android_log_print(ANDROID_LOG_INFO, "LoadTexture2D", "AImageDecoder failed, using fallback");
#else
    __android_log_print(ANDROID_LOG_INFO, "LoadTexture2D", "AImageDecoder unavailable, using fallback");
#endif

    // Fallback: return checkerboard with correct dimensions
    // Try to at least get dimensions from PNG/JPEG headers
    int w = 256, h = 256;
    int channels = 4;
    
    // Check PNG signature and get dimensions
    if (len >= 24 && buffer[0] == 137 && buffer[1] == 80 && buffer[2] == 78 && buffer[3] == 71) {
        w = ((int)buffer[16] << 24) | ((int)buffer[17] << 16) | ((int)buffer[18] << 8) | buffer[19];
        h = ((int)buffer[20] << 24) | ((int)buffer[21] << 16) | ((int)buffer[22] << 8) | buffer[23];
        __android_log_print(ANDROID_LOG_INFO, "LoadTexture2D", "PNG detected: %dx%d", w, h);
        channels = 4;
    }
    // Check JPEG signature and find SOF marker for dimensions
    else if (len >= 4 && buffer[0] == 0xFF && buffer[1] == 0xD8) {
        __android_log_print(ANDROID_LOG_INFO, "LoadTexture2D", "JPEG detected, searching for dimensions...");
        channels = 3;
        for (int pos = 2; pos + 8 < len; pos++) {
            if (buffer[pos] == 0xFF && (buffer[pos+1] == 0xC0 || buffer[pos+1] == 0xC1)) {
                if (pos + 9 < len) {
                    h = ((int)buffer[pos + 5] << 8) | buffer[pos + 6];
                    w = ((int)buffer[pos + 7] << 8) | buffer[pos + 8];
                    __android_log_print(ANDROID_LOG_INFO, "LoadTexture2D", "JPEG dimensions: %dx%d", w, h);
                    break;
                }
            }
        }
    }
    
    if (x) *x = w;
    if (y) *y = h;
    if (channels_in_file) *channels_in_file = channels;
    
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
