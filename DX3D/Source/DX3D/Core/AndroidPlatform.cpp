#if defined(DX3D_PLATFORM_ANDROID)
#include <DX3D/Core/AndroidPlatform.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <string>

namespace dx3d::platform
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
			return {};
		}
		std::string normalizedPath(path);
		const std::string prefixForward = "DX3D/Assets/";
		const std::string prefixBackward = "DX3D\\Assets\\";
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

		AAsset* asset = AAssetManager_open(g_assetManager, normalizedPath.c_str(), AASSET_MODE_BUFFER);
		if (!asset)
		{
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
}
#endif
