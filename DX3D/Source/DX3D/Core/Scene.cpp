#include <DX3D/Core/Scene.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/GraphicsDevice.h>
#include <DX3D/Graphics/Texture2D.h>
#include <imgui.h>

using namespace dx3d;

void Scene::renderImGui(GraphicsEngine& engine)
{
    // Default fallback: show a cat image if available
    static std::shared_ptr<Texture2D> s_catTexture;
    if (!s_catTexture)
    {
        s_catTexture = Texture2D::LoadTexture2D(
            engine.getGraphicsDevice().getD3DDevice(),
            L"DX3D/Assets/Textures/cat.jpg"
        );
    }

    ImGui::SetNextWindowSize(ImVec2(260, 260), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Scene UI"))
    {
        if (s_catTexture)
        {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            float size = (avail.x < avail.y) ? avail.x : avail.y;
            ImGui::Image((ImTextureID)s_catTexture->getSRV(), ImVec2(size, size));
        }
        else
        {
            ImGui::Text("No ImGui UI for this scene.");
        }
    }
    ImGui::End();
}

