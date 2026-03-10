/* TheFluidSimulations - Uses TheEngine as DLL with FLIP, SPH, and Powder scenes. */

#include <TheEngine/All.h>
#include <TheEngine/Graphics/GraphicsEngine.h>
#include <TheEngine/Game/Scenes/FlipFluidSimulationScene.h>
#include <TheEngine/Game/Scenes/SPHFluidSimulationScene.h>
#include <TheEngine/Game/Scenes/PowderScene.h>
#include <windows.h>
#include <cstdlib>
#include <cstdio>

int main(int argc, char* argv[])
{
    int sceneChoice = 1; // 1=FLIP, 2=SPH, 3=Powder
    if (argc >= 2)
    {
        sceneChoice = atoi(argv[1]);
        if (sceneChoice < 1 || sceneChoice > 3) sceneChoice = 1;
    }

    try
    {
        TheEngine::Game game({ {1280, 720}, TheEngine::Logger::LogLevel::Info });

        if (sceneChoice == 1)
            game.setScene(std::make_unique<TheEngine::FlipFluidSimulationScene>());
        else if (sceneChoice == 2)
            game.setScene(std::make_unique<TheEngine::SPHFluidSimulationScene>());
        else
            game.setScene(std::make_unique<TheEngine::PowderScene>());

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
        MessageBoxA(nullptr, "Unknown exception occurred.", "Unknown Error", MB_OK | MB_ICONERROR);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
