#pragma once
#include <DX3D/Core/EntityManager.h>
#include <DX3D/Core/Scene.h>
#include <DX3D/Graphics/Camera.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/SpriteComponent.h>
#include <DX3D/Graphics/LineRenderer.h>
#include <DX3D/Graphics/DirectWriteText.h>
#include <DX3D/Components/Quadtree.h>
#include <DX3D/Components/AABBTree.h>
#include <DX3D/Components/KDTree.h>
#include <DX3D/Components/Octree.h>
#include <DX3D/Components/ButtonComponent.h>
#include <DX3D/Components/PanelComponent.h>
#include <DX3D/Core/Input.h>
#include <DX3D/Graphics/ShadowMap.h>
#include <memory>
#include <set>
#include <vector>
#include <random>
 
namespace dx3d
{
    enum class SimulationSpeed { Paused, Normal, Fast, VeryFast };

    struct MovingEntity {
        std::string name;
        Vec2 velocity;
        Vec2 bounds;  // For bouncing off edges
        QuadtreeEntity qtEntity;
        bool active = true;
        int currentPOI = -1;  // Index of current POI being attracted to (-1 = none)
        float poiAttractionStrength = 1.0f;  // How strongly this entity is attracted to POIs
        float poiSwitchTimer = 0.0f;  // Timer for switching between POIs
    };

    struct MovingEntity3D {
        std::string name;
        Vec3 velocity;
        Vec3 bounds;  // For bouncing off edges
        Vec3 position;
        Vec3 size;
        int id;
        bool active = true;
        int currentPOI = -1;  // Index of current POI being attracted to (-1 = none)
        float poiAttractionStrength = 1.0f;  // How strongly this entity is attracted to POIs
        float poiSwitchTimer = 0.0f;  // Timer for switching between POIs
    };

    struct PointOfInterest {
        Vec2 position;
        Vec4 color;
        float attractionRadius = 100.0f;  // How far entities can "sense" this POI
        float attractionStrength = 1.0f;  // How strong the attraction is
        std::string name;
        bool active = true;
    };

    class PartitionScene : public Scene {
    public:
        void load(GraphicsEngine& engine) override;
        void update(float dt) override;
        void render(GraphicsEngine& engine, SwapChain& swapChain) override;
        void renderImGui(GraphicsEngine& engine) override;
        void fixedUpdate(float dt) override;

    private:
        std::unique_ptr<EntityManager> m_entityManager;
        std::unique_ptr<Quadtree> m_quadtree;
        std::unique_ptr<AABBTree> m_aabbTree;
        std::unique_ptr<KDTree> m_kdTree;
        std::unique_ptr<Octree> m_octree;
        LineRenderer* m_lineRenderer = nullptr;
        TextComponent* m_entityCountText = nullptr;
        TextComponent* m_dbscanEpsText = nullptr;
        TextComponent* m_dbscanMinPtsText = nullptr;
        TextComponent* m_dbscanClustersText = nullptr;
        
        // Offset control UI elements
        TextComponent* m_offsetXText = nullptr;
        TextComponent* m_offsetYText = nullptr;
        
        // POI status UI elements
        PanelComponent* m_poiStatusPanel = nullptr;
        TextComponent* m_poiStatusText = nullptr;
        TextComponent* m_poiCountText = nullptr;
        TextComponent* m_poiStrengthText = nullptr;
        TextComponent* m_entitySpeedText = nullptr;
        
        // K-means test data UI elements
        PanelComponent* m_kmeansDataPanel = nullptr;
        TextComponent* m_kmeansKText = nullptr;
        TextComponent* m_kmeansIterationsText = nullptr;
        TextComponent* m_kmeansConvergedText = nullptr;
        TextComponent* m_kmeansClustersText = nullptr;
        TextComponent* m_kmeansAvgDistanceText = nullptr;
        ButtonComponent* m_kmeansHullVoronoiToggle = nullptr;
        
        // DBSCAN test data UI elements
        PanelComponent* m_dbscanDataPanel = nullptr;
        
        // K-means button references for visibility control
        ButtonComponent* m_increaseKButton = nullptr;
        ButtonComponent* m_decreaseKButton = nullptr;
        ButtonComponent* m_toggleClusterVizButton = nullptr;
        
        // DBSCAN button references for visibility control
        ButtonComponent* m_increaseEpsButton = nullptr;
        ButtonComponent* m_decreaseEpsButton = nullptr;
        ButtonComponent* m_increaseMinPtsButton = nullptr;
        ButtonComponent* m_decreaseMinPtsButton = nullptr;
        ButtonComponent* m_dbscanHullVoronoiToggle = nullptr;
        ButtonComponent* m_toggle3DModeButton = nullptr;
        
        bool m_showQuadtree = true;
        int m_entityCounter = 0;
        Vec2 m_quadtreeVisualOffsetOriginal = Vec2(0, 0);
        Vec2 m_quadtreeVisualOffsetDBScan = Vec2(0, 0);

        Vec2 m_quadtreeVisualOffset = Vec2(0, 0);
        float m_offsetSpeed = 50.0f; // Speed for offset movement
        void createTestEntities(GraphicsDevice& device);
        void createUIElements(GraphicsDevice& device);
        void addRandomEntities();
        void clearAllEntities();
        void addEntityAtPosition(const Vec2& worldPos);
        void generateConcentricCirclesDataset();
        void generateConcentricCirclesDatasetLight();
        
        // POI system methods
        void addPointOfInterest(const Vec2& position, const std::string& name = "");
        void removePointOfInterest(int index);
        void clearAllPOIs();
        void updatePOIAttraction();
        void selectPOIForEntity(MovingEntity& entity);
        Vec2 calculatePOIAttractionForce(const MovingEntity& entity, const PointOfInterest& poi);
        void createDefaultPOIs();
        void updatePOIStatus();
        void updateQuadtreeVisualization();
        void createSpeedControls(GraphicsDevice& device);
        void updateSpeedControls();
        void setSimulationSpeed(dx3d::SimulationSpeed speed);
        void respawnWorldAnchorSprite();
        void updateCameraMovement(float dt);
        void updateOffsetControls(float dt);
        Vec2 screenToWorldPosition(const Vec2& screenPos);
        void updateMovingEntities(float dt);
        void updateQuadtreePartitioning();
        
        // 3D mode methods
        void toggle3DMode();
        void convertTo3D();
        void convertTo2D();
        void update3DCamera(float dt);
        void update3DMovingEntities(float dt);
        Vec3 screenToWorldPosition3D(const Vec2& screenPos);
        
        // Simple shadow mapping methods
        void renderShadowMap(GraphicsEngine& engine);
        void calculateLightViewProj();
        void createTest3DEntities(GraphicsDevice& device);
        void addRandom3DEntities(GraphicsDevice& device, int count = 5);
        void clearAllEntities3D();
        void renderShadowMapDebug(GraphicsEngine& engine);
        
        // 3D camera presets
        enum class CameraPreset { FirstPerson, TopDown, Isometric };
        void setCameraPreset(CameraPreset preset);

        std::vector<MovingEntity> m_movingEntities;
        std::vector<MovingEntity3D> m_movingEntities3D;
        std::vector<PointOfInterest> m_pointsOfInterest;
        
        // 3D mode state
        bool m_is3DMode = false;
        Camera3D m_camera3D;
        float m_cameraYaw = 0.0f;
        float m_cameraPitch = -1.57f; // Start looking down
        Vec4 m_backgroundColor = Vec4(0.27f, 0.39f, 0.55f, 1.0f); // Default dotted blue background
        bool m_showDottedBackground = true; // Show dotted background in 3D mode
        float m_dotSpacing = 40.0f; // Spacing between dots in pixels
        float m_dotRadius = 1.2f; // Radius of each dot in pixels
        
        // Simple shadow mapping
        std::unique_ptr<ShadowMap> m_shadowMap;
        std::unique_ptr<ShadowMap> m_shadowMap2; // second light
        Microsoft::WRL::ComPtr<ID3D11SamplerState> m_shadowSampler;
        Mat4 m_lightViewProj;
        Mat4 m_lightViewProj2; // second light matrix
        bool m_showShadowMapDebug = false;
        // ImGui depth preview controls
        bool m_showShadowPreview = true;
        int m_selectedShadowMap = 0; // 0: light 1, 1: light 2
        float m_shadowPreviewSize = 220.0f;
        Vec2 m_lastMouse = Vec2(0.0f, 0.0f);
        bool m_mouseCaptured = false;
        CameraPreset m_cameraPreset = CameraPreset::TopDown;
        
        // FPS Camera controls
        float m_cameraMoveSpeed = 15.0f;
        float m_cameraMouseSensitivity = 2.0f;
        float m_cameraRunMultiplier = 2.0f;
        float m_updateTimer = 0.0f;
        const float m_updateInterval = 0.016f; // ~60 FPS for quadtree updates
        bool m_entitiesMoving = true;
        
        // POI system settings
        float m_poiSwitchInterval = 5.0f;  // How often entities switch POIs (seconds)
        float m_poiAttractionStrength = 0.3f;  // Base attraction strength (scaled for inverse square law)
        float m_poiMaxDistance = 200.0f;  // Maximum distance for POI attraction
        float m_poiDamping = 0.95f;  // Velocity damping factor for smoother movement
        bool m_poiSystemEnabled = true;
        bool m_addPOIMode = false;  // Whether we're in POI creation mode
        
        // Entity speed control
        float m_entitySpeedMultiplier = 3.0f;  // Speed multiplier for entity movement
        
        // ImGui font oversampling controls
        int m_fontOversampleH = 1;
        int m_fontOversampleV = 1;
        bool m_fontNeedsRebuild = false;
        
        // Simulation speed controls
        SimulationSpeed m_simulationSpeed = SimulationSpeed::Paused;
        float m_simulationSpeedMultiplier = 0.0f;
        
        // Speed control UI elements
        ButtonComponent* m_playButton = nullptr;
        ButtonComponent* m_pauseButton = nullptr;
        ButtonComponent* m_fastForwardButton = nullptr;
        TextComponent* m_speedIndicatorText = nullptr;
        PanelComponent* m_speedControlPanel = nullptr;

        enum class PartitionType { Quadtree, AABB, KDTree };
        PartitionType m_partitionType = PartitionType::Quadtree;
        ButtonComponent* m_partitionToggleButton = nullptr;
        ButtonComponent* m_kdToggleButton = nullptr;
        PanelComponent* m_partitionStatusPanel = nullptr;
        TextComponent* m_partitionStatusText = nullptr;
        ButtonComponent* m_kdRenderModeButton = nullptr;

        // K-means clustering
        struct Cluster {
            Vec2 centroid;
            Vec4 color;
            std::vector<int> entityIndices;
        };
        
        std::vector<Cluster> m_clusters;
        bool m_kmeansEnabled = false;
        bool m_showClusterVisualization = true;
        bool m_useVoronoi = false; // Toggle between convex hulls and Voronoi
        bool m_fastMode = false;
        int m_kmeansK = 3;
        int m_kmeansIterations = 0;
        const int m_maxKmeansIterations = 20; // Reduced for faster initial clustering
        bool m_kmeansConverged = false;
        
        // DBSCAN clustering
        struct DBSCANCluster {
            Vec4 color;
            std::vector<int> entityIndices;
            int clusterId;
        };
        
        std::vector<DBSCANCluster> m_dbscanClusters;
        std::vector<DBSCANCluster> m_prevDbscanClusters; // previous frame for stable ID remapping
        int m_nextDbscanClusterId = 0; // monotonically increasing cluster IDs
        bool m_dbscanEnabled = false;
        float m_dbscanEps = 50.0f;  // Epsilon distance parameter
        int m_dbscanMinPts = 3;     // Minimum points for core point
        std::vector<int> m_dbscanEntityLabels; // -1: noise, 0+: cluster ID
        bool m_showDBSCANVisualization = true;
        bool m_dbscanUseVoronoi = false; // false=hulls, true=Voronoi
        static constexpr int DBSCAN_UNVISITED = -2; // Internally treat unvisited separately from noise
        static constexpr int DBSCAN_NOISE = -1;      // Noise label
        
        // Stability controls
        float m_kmeansUpdateTimer = 0.0f;
        const float m_kmeansUpdateInterval = 0.05f; // Update every 0.05 seconds (20 FPS) for faster response
        float m_clusterStabilityThreshold = 3.0f; // Lower threshold for more sensitive clustering
        std::vector<Vec2> m_previousCentroids;
        
        // DBSCAN update controls
        float m_dbscanUpdateTimer = 0.0f;
        const float m_dbscanUpdateInterval = 0.1f; // Throttled DBSCAN recompute interval
        
        // Entity tracking for dynamic updates
        std::vector<int> m_entityClusterAssignments; // Maps entity index to cluster index
        std::vector<float> m_entityDistancesToCentroids; // Cached distances to current centroids
        bool m_assignmentsChanged = false;
        
        void performKMeansClustering();
        void initializeKMeansCentroids();
        void assignEntitiesToClusters();
        void updateClusterCentroids();
        void updateEntityColors();
        Vec4 getClusterColor(int clusterIndex);
        float calculateDistance(const Vec2& a, const Vec2& b);
        bool shouldUpdateClustering();
        void storePreviousCentroids();
        
        // DBSCAN clustering methods
        void performDBSCANClustering();
        void remapDBSCANClusterIdsStable();
        float computeClusterIoU(const std::vector<int>& a, const std::vector<int>& b);
        void expandCluster(int entityIndex, int clusterId);
        std::vector<int> getNeighbors(int entityIndex);
        void updateDBSCANEntityColors();
        Vec4 getDBSCANClusterColor(int clusterIndex);
        void resetDBSCANLabels();
        
        // Dynamic clustering methods
        void updateEntityAssignments();
        void initializeEntityTracking();
        void ensureTrackingArraysSize();
        void updateSingleEntityAssignment(int entityIndex);
        bool hasEntityMovedSignificantly(int entityIndex);
        void smoothColorTransitions();
        
        // Quadtree optimization helpers
        int findEntityIndexByQuadtreeId(int qtEntityId);
        float calculateDistanceSquared(const Vec2& a, const Vec2& b);
        bool isEntityInCluster(int entityIndex, int clusterIndex);
        void optimizeSpatialQueries();
        
        // K-means UI control methods
        void updateKMeansButtonVisibility();
        void updateKMeansTestData();
        void updateHullVoronoiToggleVisibility();
        
        // DBSCAN UI control methods
        void updateDBSCANButtonVisibility();
        void updateDBSCANTestData();
        void updatePartitionStatusUI();
        void updatePartitionButtonsVisibility();
        void updateUIForMode();

        // KD-tree visualization settings
        bool m_kdShowSplitLines = false; // false=boxes, true=split lines

        // Convex hull utilities
        std::vector<Vec2> computeConvexHull(const std::vector<Vec2>& points);
        float cross(const Vec2& o, const Vec2& a, const Vec2& b);

        // Voronoi helpers (bounded diagram via half-plane intersection within quadtree bounds)
        struct HalfPlane { Vec2 n; float d; };
        std::vector<Vec2> clipPolygonWithHalfPlane(const std::vector<Vec2>& poly, const HalfPlane& hp);
        std::vector<Vec2> computeVoronoiCell(const Vec2& site, const std::vector<Vec2>& allSites, const Vec2& boundsCenter, const Vec2& boundsSize);
    };
}