#pragma once
#include <DX3D/Core/Core.h>
#include <DX3D/Core/Base.h>
#include <DX3D/Math/Geometry.h>
#include <DX3D/Graphics/Mesh.h>
#include <DX3D/Graphics/SpriteComponent.h> 
#include <DX3D/Graphics/Abstraction/RenderBackend.h>
#include <DX3D/Graphics/Abstraction/RenderDevice.h>
#include <DX3D/Graphics/Abstraction/RenderContext.h>
#include <DX3D/Graphics/Abstraction/RenderSwapChain.h>

namespace dx3d
{
    class IRenderPipelineState;
    class GraphicsEngine : public Base
    {
    private:
        static float m_windowHeight;
        static float m_windowWidth;
    public:
        explicit GraphicsEngine(const GraphicsEngineDesc& desc);
        ~GraphicsEngine() override;

        IRenderDevice& getGraphicsDevice() noexcept;
        IRenderContext& getContext();

        void beginFrame(IRenderSwapChain& swapChain);
        void endFrame(IRenderSwapChain& swapChain);
        static float getWindowWidth() { return m_windowWidth; }
        static float getWindowHeight() { return m_windowHeight; }
        static void setWindowWidth(float newWindowWidth) { m_windowWidth = newWindowWidth; }
        static void setWindowHeight(float newWindowHeight) { m_windowHeight = newWindowHeight; }
        IImGuiBackend& getImGuiBackend() noexcept;
        RenderBackendType getBackendType() const noexcept { return m_backendType; }
        void initializePipelines();
        IRenderPipelineState& getTextPipeline() noexcept { return *m_textPipeline; }
        IRenderPipelineState& getDefaultPipeline() noexcept { return *m_pipeline; }
        IRenderPipelineState& get3DPipeline() noexcept { return *m_pipeline3D; }
        IRenderPipelineState* getToonPipeline() noexcept { return m_toonPipeline.get(); }
        IRenderPipelineState& getShadowMapPipeline() noexcept { return *m_shadowMapPipeline; }
        IRenderPipelineState* getShadowMapDebugPipeline() noexcept { return m_shadowMapDebugPipeline.get(); }
        IRenderPipelineState* getBackgroundDotsPipeline() noexcept { return m_backgroundDotsPipeline.get(); }
        IRenderPipelineState* getLinePipeline() noexcept { return m_linePipeline.get(); }
        IRenderPipelineState* getSkyboxPipeline() noexcept { return m_skyboxPipeline.get(); }
        std::shared_ptr<Mesh> getFullscreenQuad() noexcept { return m_fullscreenQuad; }
        
        // Static function to render background dots (reusable)
        static void renderBackgroundDots(IRenderContext& context, IRenderPipelineState* backgroundDotsPipeline, 
            float screenWidth, float screenHeight, float dotSpacing = 40.0f, float dotRadius = 1.2f,
            const Vec4& baseColor = Vec4(0.27f, 0.39f, 0.55f, 1.0f), 
            const Vec4& dotColor = Vec4(0.20f, 0.32f, 0.46f, 0.6f));

    private:

        std::unique_ptr<IRenderBackend> m_renderBackend;
        std::unique_ptr<IImGuiBackend> m_imguiBackend;
        RenderDevicePtr m_graphicsDevice;
        RenderContextPtr m_deviceContext;
        RenderBackendType m_backendType{ RenderBackendType::DirectX11 };
        std::shared_ptr<IRenderPipelineState> m_pipeline;
        std::shared_ptr<IRenderPipelineState> m_textPipeline;
        std::shared_ptr<IRenderPipelineState> m_pipeline3D;
        std::shared_ptr<IRenderPipelineState> m_backgroundDotsPipeline;
        std::shared_ptr<IRenderPipelineState> m_toonPipeline;
        std::shared_ptr<IRenderPipelineState> m_shadowMapPipeline;
        std::shared_ptr<IRenderPipelineState> m_shadowMapDebugPipeline;
        std::shared_ptr<IRenderPipelineState> m_linePipeline;
        std::shared_ptr<IRenderPipelineState> m_skyboxPipeline;
        std::shared_ptr<Mesh> m_fullscreenQuad;
        bool m_pipelinesInitialized{ false };
    };

}
