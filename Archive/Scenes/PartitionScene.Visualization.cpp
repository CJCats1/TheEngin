#include <DX3D/Game/Scenes/PartitionScene.h>
#include <DX3D/Graphics/LineRenderer.h>
#include <DX3D/Graphics/SpriteComponent.h>
#include <DX3D/Math/Geometry.h>
#include <cmath>

using namespace dx3d;

void PartitionScene::updateQuadtreeVisualization() {
    if (!m_lineRenderer) return;

    m_lineRenderer->clear();
    
    // Debug: Print current state

    // Use world space rendering for quadtree visualization
    m_lineRenderer->enableScreenSpace(false);

    // Draw K-means cluster visualization if enabled
    if (m_kmeansEnabled && m_showClusterVisualization && !m_clusters.empty()) {
        // Draw cluster centroids
        for (const auto& cluster : m_clusters) {
            Vec2 visualCentroid = cluster.centroid + m_quadtreeVisualOffset;
            m_lineRenderer->addRect(visualCentroid, Vec2(15.0f, 15.0f), cluster.color, 3.0f);
            
            // Draw lines from centroid to entities in this cluster
            for (int entityIndex : cluster.entityIndices) {
                if (entityIndex < m_movingEntities.size() && m_movingEntities[entityIndex].active) {
                    Vec2 entityPos = m_movingEntities[entityIndex].qtEntity.position + m_quadtreeVisualOffset;
                    m_lineRenderer->addLine(visualCentroid, entityPos, cluster.color, 1.0f);
                }
            }

            // Draw boundary: Voronoi cells or Convex hulls depending on toggle
            if (m_useVoronoi) {
                // Compute Voronoi cell for this centroid within quadtree bounds
                Vec2 boundsCenter = Vec2(0.0f, 0.0f);
                // Match Octree/KD visualization extents: use current quadtree size (window-fitting)
                Vec2 boundsSize = m_quadtreeSize;

                // Collect all sites
                std::vector<Vec2> allSites;
                allSites.reserve(m_clusters.size());
                for (const auto& c2 : m_clusters) allSites.push_back(c2.centroid);

                auto cell = geom::computeVoronoiCell(cluster.centroid, allSites, boundsCenter, boundsSize);
                if (cell.size() >= 3) {
                    for (size_t i = 0; i < cell.size(); ++i) {
                        const Vec2 a = cell[i] + m_quadtreeVisualOffset;
                        const Vec2 b = cell[(i + 1) % cell.size()] + m_quadtreeVisualOffset;
                        m_lineRenderer->addLine(a, b, cluster.color, 2.0f);
                    }
                }
            }
            else {
                // Draw convex hull around cluster
                std::vector<Vec2> points;
                points.reserve(cluster.entityIndices.size());
                for (int entityIndex : cluster.entityIndices) {
                    if (entityIndex < m_movingEntities.size() && m_movingEntities[entityIndex].active) {
                        points.push_back(m_movingEntities[entityIndex].qtEntity.position);
                    }
                }
                if (points.size() >= 3) {
                    auto hull = geom::computeConvexHull(points);
                    if (hull.size() >= 3) {
                        for (size_t i = 0; i < hull.size(); ++i) {
                            const Vec2& a = hull[i] + m_quadtreeVisualOffset;
                            const Vec2& b = hull[(i + 1) % hull.size()] + m_quadtreeVisualOffset;
                            m_lineRenderer->addLine(a, b, cluster.color, 2.0f);
                        }
                    }
                }
            }
        }
    }
    
    // Draw DBSCAN cluster visualization if enabled
    if (m_dbscanEnabled && m_showDBSCANVisualization && !m_dbscanClusters.empty()) {
        // Precompute DBSCAN cluster centroids for Voronoi sites (if needed)
        std::vector<Vec2> dbscanCentroids;
        if (m_dbscanUseVoronoi) {
            dbscanCentroids.reserve(m_dbscanClusters.size());
            for (const auto& cluster : m_dbscanClusters) {
                Vec2 c(0.0f, 0.0f);
                int count = 0;
                for (int idx : cluster.entityIndices) {
                    if (idx >= 0 && idx < (int)m_movingEntities.size() && m_movingEntities[idx].active) {
                        c.x += m_movingEntities[idx].qtEntity.position.x;
                        c.y += m_movingEntities[idx].qtEntity.position.y;
                        ++count;
                    }
                }
                if (count > 0) {
                    c.x /= count; c.y /= count;
                }
                dbscanCentroids.push_back(c);
            }
        }

        // Draw cluster connections
        for (const auto& cluster : m_dbscanClusters) {
            if (cluster.entityIndices.size() < 2) continue; // Need at least 2 points to draw connections
            
            // Draw connections between entities in the same cluster
            for (size_t i = 0; i < cluster.entityIndices.size(); i++) {
                for (size_t j = i + 1; j < cluster.entityIndices.size(); j++) {
                    int entityIndex1 = cluster.entityIndices[i];
                    int entityIndex2 = cluster.entityIndices[j];
                    
                    if (entityIndex1 < m_movingEntities.size() && entityIndex2 < m_movingEntities.size() &&
                        m_movingEntities[entityIndex1].active && m_movingEntities[entityIndex2].active) {
                        
                        Vec2 pos1 = m_movingEntities[entityIndex1].qtEntity.position + m_quadtreeVisualOffset;
                        Vec2 pos2 = m_movingEntities[entityIndex2].qtEntity.position + m_quadtreeVisualOffset;
                        
                        // Only draw line if entities are within epsilon distance
                        if (calculateDistance(m_movingEntities[entityIndex1].qtEntity.position, 
                                             m_movingEntities[entityIndex2].qtEntity.position) <= m_dbscanEps) {
                            m_lineRenderer->addLine(pos1, pos2, cluster.color, 1.0f);
                        }
                    }
                }
            }

            // Draw convex hull around DBSCAN cluster only if Voronoi is off
            if (!m_dbscanUseVoronoi) {
                std::vector<Vec2> points;
                points.reserve(cluster.entityIndices.size());
                for (int entityIndex : cluster.entityIndices) {
                    if (entityIndex < m_movingEntities.size() && m_movingEntities[entityIndex].active) {
                        points.push_back(m_movingEntities[entityIndex].qtEntity.position);
                    }
                }
                if (points.size() >= 3) {
                    auto hull = geom::computeConvexHull(points);
                    if (hull.size() >= 3) {
                        for (size_t i = 0; i < hull.size(); ++i) {
                            const Vec2& a = hull[i] + m_quadtreeVisualOffset;
                            const Vec2& b = hull[(i + 1) % hull.size()] + m_quadtreeVisualOffset;
                            m_lineRenderer->addLine(a, b, cluster.color, 2.0f);
                        }
                    }
                }
            }
        }

        // Draw Voronoi partitions for DBSCAN clusters using their centroids (exclusive with hulls)
        if (m_dbscanUseVoronoi && !dbscanCentroids.empty()) {
            Vec2 boundsCenter = Vec2(0.0f, 0.0f);
            // Match Octree/KD visualization extents: use current quadtree size (window-fitting)
            Vec2 boundsSize = m_quadtreeSize;
            for (size_t i = 0; i < m_dbscanClusters.size(); ++i) {
                auto cell = geom::computeVoronoiCell(dbscanCentroids[i], dbscanCentroids, boundsCenter, boundsSize);
                if (cell.size() >= 3) {
                    const Vec4 color = m_dbscanClusters[i].color;
                    for (size_t e = 0; e < cell.size(); ++e) {
                        const Vec2 a = cell[e] + m_quadtreeVisualOffset;
                        const Vec2 b = cell[(e + 1) % cell.size()] + m_quadtreeVisualOffset;
                        m_lineRenderer->addLine(a, b, color, 2.0f);
                    }
                }
            }
        }
    }

    // Draw POIs
    for (const auto& poi : m_pointsOfInterest) {
        if (!poi.active) continue;
        
        Vec2 visualPos = poi.position + m_quadtreeVisualOffset;
        
        // Draw POI as a circle
        float radius = 8.0f;
        int segments = 16;
        for (int i = 0; i < segments; i++) {
            float angle1 = (2.0f * 3.14159f * i) / segments;
            float angle2 = (2.0f * 3.14159f * (i + 1)) / segments;
            
            Vec2 p1(visualPos.x + radius * std::cos(angle1), visualPos.y + radius * std::sin(angle1));
            Vec2 p2(visualPos.x + radius * std::cos(angle2), visualPos.y + radius * std::sin(angle2));
            
            m_lineRenderer->addLine(p1, p2, poi.color, 3.0f);
        }
        
        // Draw attraction radius
        float attractionRadius = poi.attractionRadius;
        for (int i = 0; i < segments; i++) {
            float angle1 = (2.0f * 3.14159f * i) / segments;
            float angle2 = (2.0f * 3.14159f * (i + 1)) / segments;
            
            Vec2 p1(visualPos.x + attractionRadius * std::cos(angle1), visualPos.y + attractionRadius * std::sin(angle1));
            Vec2 p2(visualPos.x + attractionRadius * std::cos(angle2), visualPos.y + attractionRadius * std::sin(angle2));
            
            m_lineRenderer->addLine(p1, p2, Vec4(poi.color.x, poi.color.y, poi.color.z, 0.3f), 1.0f);
        }
    }

    if (!m_showQuadtree) { 
        respawnWorldAnchorSprite(); 
        return; 
    }

    
    if (m_partitionType == PartitionType::Quadtree) {
        // Draw outer quadtree boundary first (thick red lines)
        Vec2 visualCenter = Vec2(0.0f, 0.0f) + m_quadtreeVisualOffset;
        m_lineRenderer->addRect(visualCenter, m_quadtreeSize, Vec4(1.0f, 0.0f, 0.0f, 1.0f), 2.0f); // Red color, thick lines for outer boundary
        
        // Draw Quadtree nodes
        std::vector<Quadtree*> nodes;
        m_quadtree->getAllNodes(nodes);
        for (auto* node : nodes) {
            Vec2 center = node->getCenter();
            Vec2 size = node->getSize();
            Vec2 visualCenter = center + m_quadtreeVisualOffset;
            
            // Draw all quadtree nodes with thin lines
            m_lineRenderer->addRect(visualCenter, size, Vec4(1.0f, 0.0f, 0.0f, 1.0f), 0.1f); // Red color, very thin lines
            
            const auto& entities = node->getEntities();
            for (const auto& entity : entities) {
                Vec2 visualEntityPos = entity.position + m_quadtreeVisualOffset;
                m_lineRenderer->addRect(visualEntityPos, entity.size, Vec4(0.0f, 1.0f, 0.0f, 1.0f), 0.5f); // Green color, thin lines
            }
        }
    } else if (m_partitionType == PartitionType::AABB) {
        // Draw outer AABB boundary first (thick blue lines)
        Vec2 visualCenter = Vec2(0.0f, 0.0f) + m_quadtreeVisualOffset;
        m_lineRenderer->addRect(visualCenter, m_quadtreeSize, Vec4(0.0f, 0.0f, 1.0f, 1.0f), 2.0f); // Blue color, thick lines for outer boundary
        
        // Draw AABB tree nodes
        std::vector<AABBNode*> nodes;
        m_aabbTree->getAllNodes(nodes);
        for (auto* node : nodes) {
            Vec2 visualCenter = node->center + m_quadtreeVisualOffset;
            Vec2 size = node->halfSize * 2.0f;
            
            // Draw all AABB nodes with thin lines
            m_lineRenderer->addRect(visualCenter, size, Vec4(0.0f, 0.0f, 1.0f, 1.0f), 0.1f); // Blue color, very thin lines
            
            if (node->isLeaf) {
                for (const auto& e : node->entities) {
                    Vec2 visualEntityPos = e.position + m_quadtreeVisualOffset;
                    m_lineRenderer->addRect(visualEntityPos, e.size, Vec4(1.0f, 1.0f, 0.0f, 1.0f), 0.5f); // Yellow color, thin lines
                }
            }
        }
    } else {
        // Draw outer KD-Tree boundary first (thick magenta lines)
        Vec2 visualCenter = Vec2(0.0f, 0.0f) + m_quadtreeVisualOffset;
        m_lineRenderer->addRect(visualCenter, m_quadtreeSize, Vec4(1.0f, 0.0f, 1.0f, 1.0f), 2.0f); // Magenta color, thick lines for outer boundary
        
        // Draw KD tree nodes
        std::vector<KDNode*> nodes;
        m_kdTree->getAllNodes(nodes);
        for (auto* node : nodes) {
            Vec2 visualCenter = node->center + m_quadtreeVisualOffset;
            Vec2 size = node->halfSize * 2.0f;
            Vec4 color = Vec4(1.0f, 0.0f, 1.0f, 1.0f); // Magenta color for testing
            
            // Draw all KD tree nodes with thin lines
            if (m_kdShowSplitLines && !node->isLeaf) {
                // Draw split line segment inside this node's box
                if (node->axis == 0) {
                    // vertical line at split x
                    float x = node->split + m_quadtreeVisualOffset.x;
                    float y0 = visualCenter.y - size.y * 0.5f;
                    float y1 = visualCenter.y + size.y * 0.5f;
                    m_lineRenderer->addLine(Vec2(x, y0), Vec2(x, y1), color, 0.1f); // Very thin lines
                } else {
                    // horizontal line at split y
                    float y = node->split + m_quadtreeVisualOffset.y;
                    float x0 = visualCenter.x - size.x * 0.5f;
                    float x1 = visualCenter.x + size.x * 0.5f;
                    m_lineRenderer->addLine(Vec2(x0, y), Vec2(x1, y), color, 0.1f); // Very thin lines
                }
            } else {
                m_lineRenderer->addRect(visualCenter, size, color, 0.1f); // Very thin lines
            }
            
            if (node->isLeaf) {
                for (const auto& e : node->entities) {
                    Vec2 visualEntityPos = e.position + m_quadtreeVisualOffset;
                    m_lineRenderer->addRect(visualEntityPos, e.size, Vec4(1.0f, 1.0f, 0.0f, 1.0f), 0.5f); // Thin lines
                }
            }
        }
    }
    
    // CRITICAL: Update the LineRenderer buffers after adding all lines
    m_lineRenderer->updateBuffer();
    
    respawnWorldAnchorSprite();
}

void PartitionScene::respawnWorldAnchorSprite() {
    // Remove previous anchor entity
    if (m_entityManager->findEntity("WorldOriginAnchor")) {
        m_entityManager->removeEntity("WorldOriginAnchor");
    }
    // Spawn a fresh anchor sprite in screen space
    auto& device = m_lineRenderer->getDevice();
    auto& anchorEntity = m_entityManager->createEntity("WorldOriginAnchor");
    auto& anchorSprite = anchorEntity.addComponent<SpriteComponent>(
        device,
        L"DX3D/Assets/Textures/node.png",
        1.0f,
        1.0f
    );
    // Place back at world origin in world space
    anchorSprite.setPosition(0.0f, 0.0f, 0.0f);
    anchorSprite.setTint(Vec4(0.0f, 0.0f, 0.0f, 0.0f));
    anchorSprite.setVisible(true);
}


