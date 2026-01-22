#pragma once
#include <DX3D/Core/Scene.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/Camera.h>
#include <DX3D/Graphics/SpriteComponent.h>
#include <DX3D/Core/EntityManager.h>
#include <DX3D/Graphics/LineRenderer.h>
#include <DX3D/Components/Mesh3DComponent.h>
#include <DX3D/Graphics/Mesh.h>
#include <DX3D/Graphics/Texture2D.h>
#include <DX3D/Graphics/ShadowMap.h>
#include <DX3D/Core/Input.h>
#include <DX3D/Components/SunComponent.h>
#include <memory>
#include <vector>
#include <string>

namespace dx3d
{
    class CloudScene : public Scene {
    public:
        void load(GraphicsEngine& engine) override;
        void update(float dt) override;
        void render(GraphicsEngine& engine, SwapChain& swapChain) override;
        void renderImGui(GraphicsEngine& engine) override;
        void fixedUpdate(float dt) override;

    private:
        // Core components
        std::unique_ptr<EntityManager> m_entityManager;
        Camera3D m_camera3D;
        LineRenderer* m_lineRenderer = nullptr;
        GraphicsDevice* m_graphicsDevice = nullptr;
        
        // 3D Camera controls
        float m_cameraYaw = 0.0f;
        float m_cameraPitch = 0.0f;
        float m_cameraMoveSpeed = 15.0f;
        float m_cameraMouseSensitivity = 2.0f;
        float m_cameraRunMultiplier = 2.0f;
        Vec2 m_lastMouse = Vec2(0.0f, 0.0f);
        bool m_mouseCaptured = false;
        
        // Camera presets
        enum class CameraPreset { FirstPerson, TopDown, Isometric };
        CameraPreset m_cameraPreset = CameraPreset::FirstPerson;
        void setCameraPreset(CameraPreset preset);
        
        // 3D Scene setup
        void createGroundPlane(GraphicsDevice& device);
        void createSunEntity(GraphicsDevice& device);
        void createCloudCube(GraphicsDevice& device);
        void setupLighting();
        
        // Camera controls
        void update3DCamera(float dt);
        Vec3 screenToWorldPosition3D(const Vec2& screenPos);
        
        // Shadow mapping
        std::unique_ptr<ShadowMap> m_shadowMap;
        std::unique_ptr<ShadowMap> m_shadowMap2;
        Microsoft::WRL::ComPtr<ID3D11SamplerState> m_shadowSampler;
        Mat4 m_lightViewProj;
        Mat4 m_lightViewProj2;
        bool m_enableShadowMapping = true;
        bool m_light1Shadows = true;
        bool m_light2Shadows = true;
        int m_shadowMapSize = 1024;
        bool m_softShadows = true;
        
        // Legacy light settings for backward compatibility (will be removed)
        struct LightSettings {
            bool enabled = true;
            Vec3 direction = Vec3(0.0f, -1.0f, 0.0f);
            Vec3 color = Vec3(1.0f, 1.0f, 1.0f);
            float intensity = 1.0f;
            Vec3 position = Vec3(0.0f, 50.0f, 0.0f);
            Vec3 target = Vec3(0.0f, 0.0f, 0.0f);
            float orthoSize = 100.0f;
            float nearPlane = 0.1f;
            float farPlane = 200.0f;
        };
        
        LightSettings m_light1; // TODO: Remove - use m_sun1.light* properties instead
        LightSettings m_light2; // TODO: Remove - use m_sun2.light* properties instead
        
        // Shadow mapping methods
        void renderShadowMap(GraphicsEngine& engine);
        void calculateLightViewProj();
        void createShadowSampler(ID3D11Device* device);
        void renderShadowMapDebug(GraphicsEngine& engine);
        
        // Sun animation
        void updateSunAnimation(float dt);
        
        // Background
        Vec4 m_backgroundColor = Vec4(0.27f, 0.39f, 0.55f, 1.0f); // Sky blue
        bool m_showDottedBackground = true;
        float m_dotSpacing = 40.0f;
        float m_dotRadius = 1.2f;
        
        
        // Two sun components: main and secondary
        SunComponent m_sun1;
        SunComponent m_sun2;

        // Shared sun controls
        std::string m_sunEntityName = "";
        Vec3 m_sunPosition = Vec3(100.0f, 100.0f, 100.0f); // default for sun1
        float m_sunRadius = 10.0f;
        Vec3 m_sunColor = Vec3(1.0f, 0.8f, 0.4f);
        float m_sunIntensity = 2.0f;
        float m_sunPulseSpeed = 1.0f;
        float m_sunPulseAmplitude = 0.1f;
        bool m_sunVisible = true;
        SpriteComponent* m_sunSprite = nullptr; // kept for backward compatibility (points to sun1 core)
        
        // Sun rotation controls
        float m_sunRotationX;
        float m_sunRotationY;
        float m_sunRotationZ;
        bool m_sunManualRotation;
        
        // Debug controls
        bool m_showShadowMapDebug = false;
        float m_shadowPreviewSize = 200.0f;
        int m_selectedShadowMap = 0;

        // Worley noise settings
        struct WorleyNoiseSettings {
            int seed = 1;
            int numDivisionsA = 8;
            int numDivisionsB = 15;
            int numDivisionsC = 19;
            float persistence = 0.7f;
            bool invert = false;
            int textureSize = 256;
            bool autoUpdate = true;
            
            // Advanced noise settings
            float shapeResolution = 132.0f;
            float detailResolution = 64.0f;
            float noiseScale = 1.0f;
            float noiseOffset = 0.0f;
            float noiseRotation = 0.0f;
            bool useDistance = true;
            bool useF1F2 = false;
            float f1Weight = 1.0f;
            float f2Weight = 0.5f;
            
            // Viewer settings
            bool viewerEnabled = true;
            bool viewerGreyscale = false;
            bool viewerShowAllChannels = false;
            float viewerSliceDepth = 0.638f;
            float viewerTileAmount = 1.0f;
        };
        WorleyNoiseSettings m_worleySettings;
        std::shared_ptr<Texture2D> m_worleyTexture;
        bool m_worleyTextureNeedsUpdate = true;

        // Cloud cube settings
        struct CloudCubeSettings {
            Vec3 position = Vec3(0.0f, 20.0f, 0.0f);
            Vec3 scale = Vec3(50.0f, 20.0f, 50.0f);
            Vec3 color = Vec3(0.2f, 0.2f, 0.2f); // Dark clouds
            float density = 1.0f;
            float coverage = 0.5f;
            float speed = 1.0f;
            int numSteps = 64;
            bool visible = true;
        };
        CloudCubeSettings m_cloudCubeSettings;
        Mesh3DComponent* m_cloudCubeComponent = nullptr;

        // Internals for sun management
        void initializeSuns();
        void generateWorleyNoiseTexture(GraphicsDevice& device);
        void createSimpleCloudCube(GraphicsDevice& device);
    };
}
