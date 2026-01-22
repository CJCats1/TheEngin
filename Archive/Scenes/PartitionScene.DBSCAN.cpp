#include <DX3D/Game/Scenes/PartitionScene.h>
#include <algorithm>
#include <limits>
#include <set>

using namespace dx3d;

void PartitionScene::performDBSCANClustering() {
    if (m_movingEntities.empty()) return;
    
    // Clear previous results
    // Keep a copy of previous clusters for stable remapping
    m_prevDbscanClusters = m_dbscanClusters;
    m_dbscanClusters.clear();
    resetDBSCANLabels();
    
    int nextClusterId = 0;
    
    // Process each entity
    for (int i = 0; i < (int)m_movingEntities.size(); i++) {
        if (!m_movingEntities[i].active) continue;
        if (m_dbscanEntityLabels[i] != DBSCAN_UNVISITED) continue; // already visited

        // Get neighbors including the point itself
        std::vector<int> neighbors = getNeighbors(i);

        // Core point check: |N_eps(p)| >= MinPts
        if ((int)neighbors.size() < m_dbscanMinPts) {
            m_dbscanEntityLabels[i] = DBSCAN_NOISE; // mark as noise for now (may be upgraded to border)
            continue;
        }

        // Start new cluster
        int clusterId = nextClusterId++;
        DBSCANCluster cluster;
        cluster.clusterId = clusterId;
        cluster.color = getDBSCANClusterColor(clusterId);
        cluster.entityIndices.clear();

        // Expand cluster
        expandCluster(i, clusterId);

        // Collect members for visualization
        for (int j = 0; j < (int)m_movingEntities.size(); j++) {
            if (m_movingEntities[j].active && m_dbscanEntityLabels[j] == clusterId) {
                cluster.entityIndices.push_back(j);
            }
        }

        if (!cluster.entityIndices.empty()) {
            m_dbscanClusters.push_back(cluster);
        }
    }

    // Stable ID remapping and color reuse
    remapDBSCANClusterIdsStable();
    updateDBSCANEntityColors();
    updateQuadtreeVisualization();
}

void PartitionScene::expandCluster(int entityIndex, int clusterId) {
    // BFS over density-reachable set
    std::vector<int> queue = getNeighbors(entityIndex); // includes the point itself
    // Assign the seed point
    m_dbscanEntityLabels[entityIndex] = clusterId;

    for (size_t qi = 0; qi < queue.size(); ++qi) {
        int current = queue[qi];

        // If it was noise, upgrade to border (cluster member)
        if (m_dbscanEntityLabels[current] == DBSCAN_NOISE) {
            m_dbscanEntityLabels[current] = clusterId;
        }

        // If unvisited, assign to cluster
        if (m_dbscanEntityLabels[current] == DBSCAN_UNVISITED) {
            m_dbscanEntityLabels[current] = clusterId;

            // Check if current is a core point
            std::vector<int> currentNeighbors = getNeighbors(current);
            if ((int)currentNeighbors.size() >= m_dbscanMinPts) {
                // Append neighbors to queue for further expansion
                for (int nb : currentNeighbors) {
                    if (std::find(queue.begin(), queue.end(), nb) == queue.end()) {
                        queue.push_back(nb);
                    }
                }
            }
        }
    }
}

std::vector<int> PartitionScene::getNeighbors(int entityIndex) {
    std::vector<int> neighbors;
    
    if (entityIndex < 0 || entityIndex >= m_movingEntities.size()) return neighbors;
    if (!m_movingEntities[entityIndex].active) return neighbors;
    
    Vec2 entityPos = m_movingEntities[entityIndex].qtEntity.position;
    
    // Include the point itself per the standard definition |N_eps(p)| >= MinPts
    neighbors.push_back(entityIndex);
    
    for (int i = 0; i < (int)m_movingEntities.size(); i++) {
        if (i == entityIndex || !m_movingEntities[i].active) continue;
        
        float distance = calculateDistance(entityPos, m_movingEntities[i].qtEntity.position);
        if (distance <= m_dbscanEps) {
            neighbors.push_back(i);
        }
    }
    
    return neighbors;
}

void PartitionScene::updateDBSCANEntityColors() {
    // Precompute DBSCAN centroids when using Voronoi mode to color noise by nearest partition
    std::vector<Vec2> voronoiCentroids;
    if (m_dbscanEnabled && m_dbscanUseVoronoi && !m_dbscanClusters.empty()) {
        voronoiCentroids.reserve(m_dbscanClusters.size());
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
            if (count > 0) { c.x /= count; c.y /= count; }
            voronoiCentroids.push_back(c);
        }
    }

    // Reset all entities to default color first
    for (auto& movingEntity : m_movingEntities) {
        if (auto* entity = m_entityManager->findEntity(movingEntity.name)) {
            if (auto* sprite = entity->getComponent<SpriteComponent>()) {
                sprite->setTint(Vec4(0.2f, 0.8f, 0.2f, 0.8f)); // Default green
            }
        }
    }
    
    // Color entities based on their cluster assignments
    for (int i = 0; i < m_movingEntities.size(); i++) {
        if (!m_movingEntities[i].active) continue;
        
        int label = m_dbscanEntityLabels[i];
        if (label >= 0) {
            // Find the cluster this entity belongs to
            for (const auto& cluster : m_dbscanClusters) {
                if (cluster.clusterId == label) {
                    if (auto* entity = m_entityManager->findEntity(m_movingEntities[i].name)) {
                        if (auto* sprite = entity->getComponent<SpriteComponent>()) {
                            sprite->setTint(cluster.color);
                        }
                    }
                    break;
                }
            }
        } else if (label == -1) {
            // Noise points - make them gray
            // In Voronoi mode: color noise by nearest cluster centroid to indicate partition
            if (m_dbscanEnabled && m_dbscanUseVoronoi && !voronoiCentroids.empty()) {
                Vec2 p = m_movingEntities[i].qtEntity.position;
                float best = std::numeric_limits<float>::max();
                int bestIdx = -1;
                for (int ci = 0; ci < (int)voronoiCentroids.size(); ++ci) {
                    float d = calculateDistanceSquared(p, voronoiCentroids[ci]);
                    if (d < best) { best = d; bestIdx = ci; }
                }
                if (bestIdx >= 0) {
                    if (auto* entity = m_entityManager->findEntity(m_movingEntities[i].name)) {
                        if (auto* sprite = entity->getComponent<SpriteComponent>()) {
                            sprite->setTint(m_dbscanClusters[bestIdx].color);
                        }
                    }
                    continue;
                }
            }
            // Default gray when not in Voronoi mode
            if (auto* entity = m_entityManager->findEntity(m_movingEntities[i].name)) {
                if (auto* sprite = entity->getComponent<SpriteComponent>()) {
                    sprite->setTint(Vec4(0.5f, 0.5f, 0.5f, 0.8f));
                }
            }
        }
    }
}

Vec4 PartitionScene::getDBSCANClusterColor(int clusterIndex) {
    // Generate distinct colors for DBSCAN clusters
    static std::vector<Vec4> colors = {
        Vec4(1.0f, 0.0f, 0.0f, 0.8f), // Red
        Vec4(0.0f, 1.0f, 0.0f, 0.8f), // Green
        Vec4(0.0f, 0.0f, 1.0f, 0.8f), // Blue
        Vec4(1.0f, 1.0f, 0.0f, 0.8f), // Yellow
        Vec4(1.0f, 0.0f, 1.0f, 0.8f), // Magenta
        Vec4(0.0f, 1.0f, 1.0f, 0.8f), // Cyan
        Vec4(1.0f, 0.5f, 0.0f, 0.8f), // Orange
        Vec4(0.5f, 0.0f, 1.0f, 0.8f), // Purple
        Vec4(0.8f, 0.2f, 0.2f, 0.8f), // Dark Red
        Vec4(0.2f, 0.8f, 0.2f, 0.8f)  // Dark Green
    };
    
    return colors[clusterIndex % colors.size()];
}

void PartitionScene::resetDBSCANLabels() {
    m_dbscanEntityLabels.clear();
    m_dbscanEntityLabels.resize(m_movingEntities.size(), DBSCAN_UNVISITED);
}

float PartitionScene::computeClusterIoU(const std::vector<int>& a, const std::vector<int>& b) {
    if (a.empty() && b.empty()) return 1.0f;
    if (a.empty() || b.empty()) return 0.0f;
    std::set<int> sa(a.begin(), a.end());
    std::set<int> sb(b.begin(), b.end());
    size_t inter = 0;
    for (int v : sa) if (sb.count(v)) inter++;
    size_t uni = sa.size();
    for (int v : sb) if (!sa.count(v)) uni++;
    if (uni == 0) return 0.0f;
    return static_cast<float>(inter) / static_cast<float>(uni);
}

void PartitionScene::remapDBSCANClusterIdsStable() {
    if (m_dbscanClusters.empty()) return;
    
    // Temporary ids and provisional colors
    for (size_t i = 0; i < m_dbscanClusters.size(); ++i) {
        m_dbscanClusters[i].clusterId = static_cast<int>(i);
        if (m_dbscanClusters[i].color.w == 0.0f) {
            m_dbscanClusters[i].color = getDBSCANClusterColor(static_cast<int>(i));
        }
    }
    
    const float MATCH_THRESHOLD = 0.15f;
    std::vector<int> prevAssigned(m_prevDbscanClusters.size(), 0);
    for (auto& newC : m_dbscanClusters) {
        float best = -1.0f;
        int bestIdx = -1;
        for (size_t j = 0; j < m_prevDbscanClusters.size(); ++j) {
            if (prevAssigned[j]) continue;
            float iou = computeClusterIoU(newC.entityIndices, m_prevDbscanClusters[j].entityIndices);
            if (iou > best) { best = iou; bestIdx = static_cast<int>(j); }
        }
        if (bestIdx >= 0 && best >= MATCH_THRESHOLD) {
            newC.clusterId = m_prevDbscanClusters[bestIdx].clusterId;
            newC.color = m_prevDbscanClusters[bestIdx].color;
            prevAssigned[bestIdx] = 1;
        } else {
            newC.clusterId = m_nextDbscanClusterId++;
            newC.color = getDBSCANClusterColor(newC.clusterId);
        }
    }
    
    // Rewrite labels to final ids
    for (int i = 0; i < static_cast<int>(m_movingEntities.size()); ++i) {
        if (!m_movingEntities[i].active) continue;
        int label = m_dbscanEntityLabels[i];
        if (label >= 0) {
            for (const auto& c : m_dbscanClusters) {
                if (std::find(c.entityIndices.begin(), c.entityIndices.end(), i) != c.entityIndices.end()) {
                    m_dbscanEntityLabels[i] = c.clusterId;
                    break;
                }
            }
        }
    }
}


