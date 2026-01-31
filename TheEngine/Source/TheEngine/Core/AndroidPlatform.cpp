#if defined(THEENGINE_PLATFORM_ANDROID)
#include <TheEngine/Core/AndroidPlatform.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <jni.h>
#include <string>

namespace TheEngine::platform
{
	namespace
	{
		AAssetManager* g_assetManager = nullptr;
		ANativeWindow* g_nativeWindow = nullptr;
		android_app* g_androidApp = nullptr;
	}

	void setAssetManager(AAssetManager* manager)
	{
		g_assetManager = manager;
	}

	AAssetManager* getAssetManager()
	{
		return g_assetManager;
	}

	std::vector<std::uint8_t> readAsset(const char* path)
	{
		if (!g_assetManager || !path)
		{
			__android_log_print(ANDROID_LOG_ERROR, "readAsset", "Asset manager not initialized or path is null");
			return {};
		}
		std::string normalizedPath(path);
		const std::string prefixForward = "TheEngine/Assets/";
		const std::string prefixBackward = "TheEngine\\Assets\\";
		if (normalizedPath.rfind(prefixForward, 0) == 0)
		{
			normalizedPath = normalizedPath.substr(prefixForward.size());
		}
		else if (normalizedPath.rfind(prefixBackward, 0) == 0)
		{
			normalizedPath = normalizedPath.substr(prefixBackward.size());
		}
		while (!normalizedPath.empty() && (normalizedPath.front() == '/' || normalizedPath.front() == '\\'))
		{
			normalizedPath.erase(normalizedPath.begin());
		}

		// Only log normalized path to reduce verbosity (original path can be inferred)
		// __android_log_print(ANDROID_LOG_DEBUG, "readAsset", "Loading asset: %s", normalizedPath.c_str());

		AAsset* asset = AAssetManager_open(g_assetManager, normalizedPath.c_str(), AASSET_MODE_BUFFER);
		if (!asset)
		{
			__android_log_print(ANDROID_LOG_ERROR, "readAsset", "Failed to open asset: %s (normalized: %s)", path, normalizedPath.c_str());
			return {};
		}
		const size_t length = AAsset_getLength(asset);
		std::vector<std::uint8_t> buffer(length);
		const int64_t readSize = AAsset_read(asset, buffer.data(), buffer.size());
		AAsset_close(asset);
		if (readSize <= 0)
		{
			return {};
		}
		if (static_cast<size_t>(readSize) != buffer.size())
		{
			buffer.resize(static_cast<size_t>(readSize));
		}
		return buffer;
	}

	void setNativeWindow(ANativeWindow* window)
	{
		g_nativeWindow = window;
	}

	ANativeWindow* getNativeWindow()
	{
		return g_nativeWindow;
	}

	void setAndroidApp(android_app* app)
	{
		g_androidApp = app;
	}

	android_app* getAndroidApp()
	{
		return g_androidApp;
	}

	unsigned char* decodeImageFromMemory(const unsigned char* buffer, int bufferSize, int* outWidth, int* outHeight, int* outChannels)
	{
		if (!g_androidApp || !buffer || bufferSize <= 0)
		{
			if (outWidth) *outWidth = 0;
			if (outHeight) *outHeight = 0;
			if (outChannels) *outChannels = 0;
			return nullptr;
		}

		JavaVM* javaVM = g_androidApp->activity->vm;
		JNIEnv* env = nullptr;
		
		jint result = javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
		if (result == JNI_ERR)
		{
			__android_log_print(ANDROID_LOG_ERROR, "decodeImage", "Failed to get JNI environment");
			return nullptr;
		}
		
		if (result == JNI_EDETACHED)
		{
			if (javaVM->AttachCurrentThread(&env, nullptr) != JNI_OK)
			{
				__android_log_print(ANDROID_LOG_ERROR, "decodeImage", "Failed to attach thread");
				return nullptr;
			}
		}

		// Create byte array from buffer
		jbyteArray byteArray = env->NewByteArray(bufferSize);
		if (!byteArray)
		{
			__android_log_print(ANDROID_LOG_ERROR, "decodeImage", "Failed to create byte array");
			if (result == JNI_EDETACHED) javaVM->DetachCurrentThread();
			return nullptr;
		}
		env->SetByteArrayRegion(byteArray, 0, bufferSize, reinterpret_cast<const jbyte*>(buffer));

		// Get BitmapFactory class
		jclass bitmapFactoryClass = env->FindClass("android/graphics/BitmapFactory");
		if (!bitmapFactoryClass)
		{
			__android_log_print(ANDROID_LOG_ERROR, "decodeImage", "Failed to find BitmapFactory class");
			env->DeleteLocalRef(byteArray);
			if (result == JNI_EDETACHED) javaVM->DetachCurrentThread();
			return nullptr;
		}

		// Call BitmapFactory.decodeByteArray
		jmethodID decodeMethod = env->GetStaticMethodID(bitmapFactoryClass, "decodeByteArray", "([BII)Landroid/graphics/Bitmap;");
		if (!decodeMethod)
		{
			__android_log_print(ANDROID_LOG_ERROR, "decodeImage", "Failed to find decodeByteArray method");
			env->DeleteLocalRef(bitmapFactoryClass);
			env->DeleteLocalRef(byteArray);
			if (result == JNI_EDETACHED) javaVM->DetachCurrentThread();
			return nullptr;
		}

		jobject bitmap = env->CallStaticObjectMethod(bitmapFactoryClass, decodeMethod, byteArray, 0, bufferSize);
		env->DeleteLocalRef(byteArray);
		
		if (!bitmap)
		{
			__android_log_print(ANDROID_LOG_ERROR, "decodeImage", "BitmapFactory.decodeByteArray returned null");
			env->DeleteLocalRef(bitmapFactoryClass);
			if (result == JNI_EDETACHED) javaVM->DetachCurrentThread();
			return nullptr;
		}

		// Get Bitmap class and methods
		jclass bitmapClass = env->GetObjectClass(bitmap);
		if (!bitmapClass)
		{
			__android_log_print(ANDROID_LOG_ERROR, "decodeImage", "Failed to get Bitmap class");
			env->DeleteLocalRef(bitmap);
			env->DeleteLocalRef(bitmapFactoryClass);
			if (result == JNI_EDETACHED) javaVM->DetachCurrentThread();
			return nullptr;
		}

		// Get width and height
		jmethodID getWidthMethod = env->GetMethodID(bitmapClass, "getWidth", "()I");
		jmethodID getHeightMethod = env->GetMethodID(bitmapClass, "getHeight", "()I");
		jmethodID getConfigMethod = env->GetMethodID(bitmapClass, "getConfig", "()Landroid/graphics/Bitmap$Config;");
		
		if (!getWidthMethod || !getHeightMethod || !getConfigMethod)
		{
			__android_log_print(ANDROID_LOG_ERROR, "decodeImage", "Failed to find Bitmap methods");
			env->DeleteLocalRef(bitmapClass);
			env->DeleteLocalRef(bitmap);
			env->DeleteLocalRef(bitmapFactoryClass);
			if (result == JNI_EDETACHED) javaVM->DetachCurrentThread();
			return nullptr;
		}

		int width = env->CallIntMethod(bitmap, getWidthMethod);
		int height = env->CallIntMethod(bitmap, getHeightMethod);
		
		if (width <= 0 || height <= 0)
		{
			__android_log_print(ANDROID_LOG_ERROR, "decodeImage", "Invalid bitmap dimensions: %dx%d", width, height);
			env->DeleteLocalRef(bitmapClass);
			env->DeleteLocalRef(bitmap);
			env->DeleteLocalRef(bitmapFactoryClass);
			if (result == JNI_EDETACHED) javaVM->DetachCurrentThread();
			return nullptr;
		}

		// Convert to ARGB_8888 format if needed
		jclass configClass = env->FindClass("android/graphics/Bitmap$Config");
		jfieldID argb8888Field = env->GetStaticFieldID(configClass, "ARGB_8888", "Landroid/graphics/Bitmap$Config;");
		jobject argb8888Config = env->GetStaticObjectField(configClass, argb8888Field);
		
		jmethodID copyMethod = env->GetMethodID(bitmapClass, "copy", "(Landroid/graphics/Bitmap$Config;Z)Landroid/graphics/Bitmap;");
		if (copyMethod && argb8888Config)
		{
			jobject newBitmap = env->CallObjectMethod(bitmap, copyMethod, argb8888Config, JNI_FALSE);
			if (newBitmap)
			{
				env->DeleteLocalRef(bitmap);
				bitmap = newBitmap;
				bitmapClass = env->GetObjectClass(bitmap);
			}
			env->DeleteLocalRef(argb8888Config);
		}
		env->DeleteLocalRef(configClass);

		// Copy pixels
		size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);
		size_t pixelBufferSize = pixelCount * 4; // RGBA = 4 bytes per pixel
		unsigned char* pixels = (unsigned char*)malloc(pixelBufferSize);
		
		if (!pixels)
		{
			__android_log_print(ANDROID_LOG_ERROR, "decodeImage", "Failed to allocate pixel buffer");
			env->DeleteLocalRef(bitmapClass);
			env->DeleteLocalRef(bitmap);
			env->DeleteLocalRef(bitmapFactoryClass);
			if (result == JNI_EDETACHED) javaVM->DetachCurrentThread();
			return nullptr;
		}

		// Get pixel data using getPixels (most reliable method)
		jintArray pixelArray = env->NewIntArray(pixelCount);
		if (!pixelArray)
		{
			__android_log_print(ANDROID_LOG_ERROR, "decodeImage", "Failed to create int array");
			free(pixels);
			env->DeleteLocalRef(bitmapClass);
			env->DeleteLocalRef(bitmap);
			env->DeleteLocalRef(bitmapFactoryClass);
			if (result == JNI_EDETACHED) javaVM->DetachCurrentThread();
			return nullptr;
		}
		
		jmethodID getPixelsMethod = env->GetMethodID(bitmapClass, "getPixels", "([IIIIIII)V");
		if (!getPixelsMethod)
		{
			__android_log_print(ANDROID_LOG_ERROR, "decodeImage", "Failed to find getPixels method");
			env->DeleteLocalRef(pixelArray);
			free(pixels);
			env->DeleteLocalRef(bitmapClass);
			env->DeleteLocalRef(bitmap);
			env->DeleteLocalRef(bitmapFactoryClass);
			if (result == JNI_EDETACHED) javaVM->DetachCurrentThread();
			return nullptr;
		}
		
		// Call getPixels: getPixels(int[] pixels, int offset, int stride, int x, int y, int width, int height)
		env->CallVoidMethod(bitmap, getPixelsMethod, pixelArray, 0, width, 0, 0, width, height);
		
		jint* pixelData = env->GetIntArrayElements(pixelArray, nullptr);
		if (pixelData)
		{
			// Convert ARGB int to RGBA bytes
			// Android Bitmap stores pixels as ARGB (A in high byte, RGB in lower 3 bytes)
			for (size_t i = 0; i < pixelCount; i++)
			{
				unsigned int argb = static_cast<unsigned int>(pixelData[i]);
				pixels[i * 4 + 0] = (argb >> 16) & 0xFF; // R (from bits 16-23)
				pixels[i * 4 + 1] = (argb >> 8) & 0xFF;  // G (from bits 8-15)
				pixels[i * 4 + 2] = argb & 0xFF;          // B (from bits 0-7)
				pixels[i * 4 + 3] = (argb >> 24) & 0xFF; // A (from bits 24-31)
			}
			env->ReleaseIntArrayElements(pixelArray, pixelData, JNI_ABORT);
		}
		else
		{
			__android_log_print(ANDROID_LOG_ERROR, "decodeImage", "Failed to get pixel data");
			free(pixels);
			env->DeleteLocalRef(pixelArray);
			env->DeleteLocalRef(bitmapClass);
			env->DeleteLocalRef(bitmap);
			env->DeleteLocalRef(bitmapFactoryClass);
			if (result == JNI_EDETACHED) javaVM->DetachCurrentThread();
			return nullptr;
		}
		
		env->DeleteLocalRef(pixelArray);

		env->DeleteLocalRef(bitmapClass);
		env->DeleteLocalRef(bitmap);
		env->DeleteLocalRef(bitmapFactoryClass);
		
		if (result == JNI_EDETACHED) javaVM->DetachCurrentThread();

		if (outWidth) *outWidth = width;
		if (outHeight) *outHeight = height;
		if (outChannels) *outChannels = 4; // RGBA

		__android_log_print(ANDROID_LOG_INFO, "decodeImage", "Successfully decoded image: %dx%d", width, height);
		return pixels;
	}
}
#endif
