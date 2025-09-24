#pragma once
#include <DX3D/Core/EntityManager.h>
#include <DX3D/Core/Scene.h>
#include <DX3D/Graphics/Camera.h>
#include <DX3D/Graphics/GraphicsEngine.h>
#include <DX3D/Graphics/SpriteComponent.h>
#include <DX3D/Graphics/LineRenderer.h>
#include <DX3D/Components/Quadtree.h>
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
        LineRenderer* m_lineRenderer = nullptr;
        bool m_showQuadtree = true;
        int m_entityCounter = 0;
        Vec2 m_quadtreeVisualOffset = Vec2(500,0);
        void createTestEntities(GraphicsDevice& device);
        void createUIElements(GraphicsDevice& device);
        void addRandomEntities();
        void clearAllEntities();
        void addEntityAtPosition(const Vec2& worldPos);
        void updateQuadtreeVisualization();
        void updateCameraMovement(float dt);
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

        // K-means clustering
        struct Cluster {
            Vec2 centroid;
            Vec4 color;
            std::vector<int> entityIndices;
        };
        
        std::vector<Cluster> m_clusters;
        bool m_kmeansEnabled = false;
        bool m_showClusterVisualization = true;
        bool m_fastMode = false;
        int m_kmeansK = 3;
        int m_kmeansIterations = 0;
        const int m_maxKmeansIterations = 20; // Reduced for faster initial clustering
        bool m_kmeansConverged = false;
        
        // Stability controls
        float m_kmeansUpdateTimer = 0.0f;
        const float m_kmeansUpdateInterval = 0.05f; // Update every 0.05 seconds (20 FPS) for faster response
        float m_clusterStabilityThreshold = 3.0f; // Lower threshold for more sensitive clustering
        std::vector<Vec2> m_previousCentroids;
        
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
    };
}