/*MIT License

C++ 3D Game Tutorial Series (https://github.com/PardCode/CPP-3D-Game-Tutorial-Series)

Copyright (c) 2019-2025, PardCode

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/

#include <TheEngine/All.h>
#include <TheEngine/Graphics/GraphicsEngine.h>
#include "Scenes/TestScenes/TestScene.h"
#include <windows.h>

int main()
{
	try
	{
		TheEngine::Game game({ {1280,720}, TheEngine::Logger::LogLevel::Info });
		game.setScene(std::make_unique<TheEngine::TestScene>());
		game.run();
	}
	catch (const std::runtime_error& e)
	{
		MessageBoxA(nullptr, e.what(), "Runtime Error", MB_OK | MB_ICONERROR);
		return EXIT_FAILURE;
	}
	catch (const std::invalid_argument& e)
	{
		MessageBoxA(nullptr, e.what(), "Invalid Argument Error", MB_OK | MB_ICONERROR);
		return EXIT_FAILURE;
	}
	catch (const std::exception& e)
	{
		MessageBoxA(nullptr, e.what(), "Exception", MB_OK | MB_ICONERROR);
		return EXIT_FAILURE;
	}
	catch (...)
	{
		MessageBoxA(nullptr, "Unknown exception occurred", "Unknown Error", MB_OK | MB_ICONERROR);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}