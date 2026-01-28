#pragma once

#if defined(DX3D_PLATFORM_ANDROID)
#include <android/asset_manager.h>
#include <android/native_window.h>
#include <android_native_app_glue.h>
#include <cstdint>
#include <vector>

namespace dx3d::platform
{
	void setAssetManager(AAssetManager* manager);
	AAssetManager* getAssetManager();
	std::vector<std::uint8_t> readAsset(const char* path);
	void setNativeWindow(ANativeWindow* window);
	ANativeWindow* getNativeWindow();
	void setAndroidApp(android_app* app);
	android_app* getAndroidApp();
	
	// Decode image using Android BitmapFactory via JNI
	// Returns decoded RGBA pixels, width, height, and channels (4 for RGBA)
	// Caller must free the returned pixel buffer
	unsigned char* decodeImageFromMemory(const unsigned char* buffer, int bufferSize, int* outWidth, int* outHeight, int* outChannels);
}
#endif
