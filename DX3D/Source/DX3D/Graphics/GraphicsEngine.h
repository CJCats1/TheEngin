#pragma once
#include <DX3D/Core/Core.h>
#include <DX3D/Core/Base.h>
#include <DX3D/Math/Geometry.h>
#include <DX3D/Graphics/Mesh.h>
#include <DX3D/Graphics/SpriteComponent.h> 

namespace dx3d
{
    class GraphicsEngine : public Base
    {
    private:
        static float m_windowHeight;
        static float m_windowWidth;
    public:
        explicit GraphicsEngine(const GraphicsEngineDesc& desc);
        ~GraphicsEngine() override;

        GraphicsDevice& getGraphicsDevice() noexcept;
        DeviceContext& getContext();

        void beginFrame(SwapChain& swapChain);
        void endFrame(SwapChain& swapChain);
        static float getWindowWidth() { return m_windowWidth; }
        static float getWindowHeight() { return m_windowHeight; }
        static void setWindowWidth(float newWindowWidth) { m_windowWidth = newWindowWidth; }
        static void setWindowHeight(float newWindowHeight) { m_windowHeight = newWindowHeight; }
        GraphicsPipelineState& getTextPipeline() noexcept { return *m_textPipeline; }
        GraphicsPipelineState& getDefaultPipeline() noexcept { return *m_pipeline; }
        GraphicsPipelineState& get3DPipeline() noexcept { return *m_pipeline3D; }
        GraphicsPipelineState* getToonPipeline() noexcept { return m_toonPipeline.get(); }
        GraphicsPipelineState& getShadowMapPipeline() noexcept { return *m_shadowMapPipeline; }
        GraphicsPipelineState* getShadowMapDebugPipeline() noexcept { return m_shadowMapDebugPipeline.get(); }
        GraphicsPipelineState* getBackgroundDotsPipeline() noexcept { return m_backgroundDotsPipeline.get(); }
        std::shared_ptr<Mesh> getFullscreenQuad() noexcept { return m_fullscreenQuad; }
        
        // Static function to render background dots (reusable)
        static void renderBackgroundDots(DeviceContext& context, GraphicsPipelineState* backgroundDotsPipeline, 
            float screenWidth, float screenHeight, float dotSpacing = 40.0f, float dotRadius = 1.2f,
            const Vec4& baseColor = Vec4(0.27f, 0.39f, 0.55f, 1.0f), 
            const Vec4& dotColor = Vec4(0.20f, 0.32f, 0.46f, 0.6f));

    private:

        std::shared_ptr<GraphicsDevice> m_graphicsDevice;
        std::shared_ptr<DeviceContext> m_deviceContext;
        std::shared_ptr<GraphicsPipelineState> m_pipeline;
        std::shared_ptr<GraphicsPipelineState> m_textPipeline;
        std::shared_ptr<GraphicsPipelineState> m_pipeline3D;
        std::shared_ptr<GraphicsPipelineState> m_backgroundDotsPipeline;
        std::shared_ptr<GraphicsPipelineState> m_toonPipeline;
        std::shared_ptr<GraphicsPipelineState> m_shadowMapPipeline;
        std::shared_ptr<GraphicsPipelineState> m_shadowMapDebugPipeline;
        std::shared_ptr<Mesh> m_fullscreenQuad;
    };

}
