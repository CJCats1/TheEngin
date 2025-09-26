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
#include <DX3D/Components/ButtonComponent.h>
#include <DX3D/Components/PanelComponent.h>
#include <DX3D/Core/Input.h>
#include <memory>
#include <set>
#include <vector>
#include <random>
 
namespace dx3d
{
    class PartitionScene : public Scene {
    public:
        void load(GraphicsEngine& engine) override;
        void update(float dt) override;
        void render(GraphicsEngine& engine, SwapChain& swapChain) override;
        void fixedUpdate(float dt) override;

    private:
        std::unique_ptr<EntityManager> m_entityManager;
        std::unique_ptr<Quadtree> m_quadtree;
        std::unique_ptr<AABBTree> m_aabbTree;
        std::unique_ptr<KDTree> m_kdTree;
        LineRenderer* m_lineRenderer = nullptr;
        TextComponent* m_entityCountText = nullptr;
        TextComponent* m_dbscanEpsText = nullptr;
        TextComponent* m_dbscanMinPtsText = nullptr;
        TextComponent* m_dbscanClustersText = nullptr;
        
        // Offset control UI elements
        TextComponent* m_offsetXText = nullptr;
        TextComponent* m_offsetYText = nullptr;
        
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
        void updateQuadtreeVisualization();
        void respawnWorldAnchorSprite();
        void updateCameraMovement(float dt);
        void updateOffsetControls(float dt);
        Vec2 screenToWorldPosition(const Vec2& screenPos);
        void updateMovingEntities(float dt);
        void updateQuadtreePartitioning();
        struct MovingEntity {
            std::string name;
            Vec2 velocity;
            Vec2 bounds;  // For bouncing off edges
            QuadtreeEntity qtEntity;
            bool active = true;
        };

        std::vector<MovingEntity> m_movingEntities;
        float m_updateTimer = 0.0f;
        const float m_updateInterval = 0.016f; // ~60 FPS for quadtree updates
        bool m_entitiesMoving = false;

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