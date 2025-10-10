#include <DX3D/Game/Scenes/PartitionScene.h>
#include <algorithm>
#include <limits>
#include <random>

using namespace dx3d;

void PartitionScene::performKMeansClustering() {
    if (m_movingEntities.empty()) return;
    
    m_kmeansIterations = 0;
    m_kmeansConverged = false;
    
    // Initialize clusters
    m_clusters.clear();
    m_clusters.resize(m_kmeansK);
    
    // Initialize entity tracking
    initializeEntityTracking();
    
    initializeKMeansCentroids();
    
    // Perform K-means iterations
    while (m_kmeansIterations < m_maxKmeansIterations && !m_kmeansConverged) {
        assignEntitiesToClusters();
        updateClusterCentroids();
        m_kmeansIterations++;
    }
    
    // Update entity colors based on cluster assignments
    updateEntityColors();
    
    // Store current centroids for stability checking
    storePreviousCentroids();
    
    // Update quadtree visualization to show cluster lines
    updateQuadtreeVisualization();
}

void PartitionScene::initializeKMeansCentroids() {
    // Use existing centroids if available and stable
    if (!m_previousCentroids.empty() && m_previousCentroids.size() == m_kmeansK) {
        for (int i = 0; i < m_kmeansK; i++) {
            m_clusters[i].centroid = m_previousCentroids[i];
            m_clusters[i].color = getClusterColor(i);
            m_clusters[i].entityIndices.clear();
        }
        return;
    }
    
    // Initialize with random positions, but try to spread them out
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> xDist(-350.0f, 350.0f);
    std::uniform_real_distribution<float> yDist(-250.0f, 250.0f);
    
    for (int i = 0; i < m_kmeansK; i++) {
        // Try to place centroids away from existing ones
        Vec2 newCentroid;
        int attempts = 0;
        do {
            newCentroid = Vec2(xDist(gen), yDist(gen));
            attempts++;
        } while (attempts < 10 && [&]() {
            for (int j = 0; j < i; j++) {
                if (calculateDistance(newCentroid, m_clusters[j].centroid) < 100.0f) {
                    return true; // Too close to existing centroid
                }
            }
            return false;
        }());
        
        m_clusters[i].centroid = newCentroid;
        m_clusters[i].color = getClusterColor(i);
        m_clusters[i].entityIndices.clear();
    }
}

void PartitionScene::assignEntitiesToClusters() {
    // Clear previous assignments
    for (auto& cluster : m_clusters) {
        cluster.entityIndices.clear();
    }
    
    // Reset entity tracking
    std::fill(m_entityClusterAssignments.begin(), m_entityClusterAssignments.end(), -1);
    std::fill(m_entityDistancesToCentroids.begin(), m_entityDistancesToCentroids.end(), std::numeric_limits<float>::max());
    
    // Use quadtree for optimized spatial assignment
    if (m_quadtree) {
        // For each entity, use quadtree to find the closest cluster centroid
        for (int i = 0; i < m_movingEntities.size(); i++) {
            if (!m_movingEntities[i].active) continue;
            
            Vec2 entityPos = m_movingEntities[i].qtEntity.position;
            float minDistance = std::numeric_limits<float>::max();
            int closestCluster = 0;
            
            // Use quadtree to find nearby clusters more efficiently
            // Calculate dynamic search radius based on current cluster distribution
            float maxClusterDistance = 0.0f;
            for (int j = 0; j < m_kmeansK; j++) {
                float distanceSquared = calculateDistanceSquared(entityPos, m_clusters[j].centroid);
                float distance = std::sqrt(distanceSquared);
                if (distance < minDistance) {
                    minDistance = distance;
                    closestCluster = j;
                }
                maxClusterDistance = std::max(maxClusterDistance, distance);
            }
            
            // Use quadtree to verify if there are any closer entities that might affect cluster boundaries
            float searchRadius = std::min(maxClusterDistance * 0.5f, 300.0f); // Dynamic search radius
            auto nearbyEntities = m_quadtree->query(entityPos, Vec2(searchRadius, searchRadius));
            
            // Check if any nearby entities are in different clusters that might be closer
            for (const auto& qtEntity : nearbyEntities) {
                // Use helper method to find entity index
                int entityIdx = findEntityIndexByQuadtreeId(qtEntity.id);
                if (entityIdx == -1) continue;
                
                // Find which cluster this entity belongs to
                for (int clusterIdx = 0; clusterIdx < m_kmeansK; clusterIdx++) {
                    if (isEntityInCluster(entityIdx, clusterIdx)) {
                        float distanceToNearbyCluster = calculateDistance(entityPos, m_clusters[clusterIdx].centroid);
                        if (distanceToNearbyCluster < minDistance) {
                            minDistance = distanceToNearbyCluster;
                            closestCluster = clusterIdx;
                        }
                        break;
                    }
                }
            }
            
            // Assign entity to closest cluster
            m_clusters[closestCluster].entityIndices.push_back(i);
            m_entityClusterAssignments[i] = closestCluster;
            m_entityDistancesToCentroids[i] = minDistance;
        }
    } else {
        // Fallback to brute force assignment
        for (int i = 0; i < m_movingEntities.size(); i++) {
            if (!m_movingEntities[i].active) continue;
            
            float minDistance = std::numeric_limits<float>::max();
            int closestCluster = 0;
            
            for (int j = 0; j < m_kmeansK; j++) {
                float distance = calculateDistance(m_movingEntities[i].qtEntity.position, m_clusters[j].centroid);
                if (distance < minDistance) {
                    minDistance = distance;
                    closestCluster = j;
                }
            }
            
            m_clusters[closestCluster].entityIndices.push_back(i);
            m_entityClusterAssignments[i] = closestCluster;
            m_entityDistancesToCentroids[i] = minDistance;
        }
    }
}

void PartitionScene::updateClusterCentroids() {
    bool converged = true;
    
    for (int i = 0; i < m_kmeansK; i++) {
        if (m_clusters[i].entityIndices.empty()) continue;
        
        Vec2 newCentroid(0.0f, 0.0f);
        int validEntityCount = 0;
        
        // Use quadtree to optimize centroid calculation for large clusters
        if (m_quadtree && m_clusters[i].entityIndices.size() > 10) {
            // Use quadtree to find entities in the cluster's spatial region
            Vec2 clusterCenter = m_clusters[i].centroid;
            float clusterRadius = 0.0f;
            
            // Calculate cluster radius based on current entities
            for (int entityIndex : m_clusters[i].entityIndices) {
                if (entityIndex < m_movingEntities.size() && m_movingEntities[entityIndex].active) {
                    float distance = calculateDistance(clusterCenter, m_movingEntities[entityIndex].qtEntity.position);
                    clusterRadius = std::max(clusterRadius, distance);
                }
            }
            
            // Use quadtree to find entities in the cluster region
            Vec2 searchSize(clusterRadius * 1.5f, clusterRadius * 1.5f);
            auto nearbyEntities = m_quadtree->query(clusterCenter, searchSize);
            
            // Calculate centroid from quadtree results
            for (const auto& qtEntity : nearbyEntities) {
                // Use helper method to find entity index
                int entityIndex = findEntityIndexByQuadtreeId(qtEntity.id);
                if (entityIndex != -1 && isEntityInCluster(entityIndex, i)) {
                    newCentroid.x += qtEntity.position.x;
                    newCentroid.y += qtEntity.position.y;
                    validEntityCount++;
                }
            }
        } else {
            // Standard centroid calculation for small clusters
            for (int entityIndex : m_clusters[i].entityIndices) {
                if (entityIndex < m_movingEntities.size() && m_movingEntities[entityIndex].active) {
                    newCentroid.x += m_movingEntities[entityIndex].qtEntity.position.x;
                    newCentroid.y += m_movingEntities[entityIndex].qtEntity.position.y;
                    validEntityCount++;
                }
            }
        }
        
        if (validEntityCount > 0) {
            newCentroid.x /= validEntityCount;
            newCentroid.y /= validEntityCount;
            
            // Check for convergence
            float distance = calculateDistance(m_clusters[i].centroid, newCentroid);
            float convergenceThreshold = m_fastMode ? 0.1f : 0.05f; // More lenient in fast mode
            if (distance > convergenceThreshold) {
                converged = false;
            }
            
            m_clusters[i].centroid = newCentroid;
        }
    }
    
    m_kmeansConverged = converged;
}

void PartitionScene::updateEntityColors() {
    for (int i = 0; i < m_kmeansK; i++) {
        for (int entityIndex : m_clusters[i].entityIndices) {
            if (entityIndex < 0 || entityIndex >= m_movingEntities.size()) continue;
            
            if (auto* entity = m_entityManager->findEntity(m_movingEntities[entityIndex].name)) {
                if (auto* sprite = entity->getComponent<SpriteComponent>()) {
                    // Immediate color update for cluster assignments
                    sprite->setTint(m_clusters[i].color);
                }
            }
        }
    }
}

Vec4 PartitionScene::getClusterColor(int clusterIndex) {
    // Generate distinct colors for clusters
    static std::vector<Vec4> colors = {
        Vec4(1.0f, 0.0f, 0.0f, 0.8f), // Red
        Vec4(0.0f, 1.0f, 0.0f, 0.8f), // Green
        Vec4(0.0f, 0.0f, 1.0f, 0.8f), // Blue
        Vec4(1.0f, 1.0f, 0.0f, 0.8f), // Yellow
        Vec4(1.0f, 0.0f, 1.0f, 0.8f), // Magenta
        Vec4(0.0f, 1.0f, 1.0f, 0.8f), // Cyan
        Vec4(1.0f, 0.5f, 0.0f, 0.8f), // Orange
        Vec4(0.5f, 0.0f, 1.0f, 0.8f)  // Purple
    };
    
    return colors[clusterIndex % colors.size()];
}

float PartitionScene::calculateDistance(const Vec2& a, const Vec2& b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

bool PartitionScene::shouldUpdateClustering() {
    if (m_clusters.empty() || m_previousCentroids.empty()) {
        return true; // First time or no previous data
    }
    
    if (m_clusters.size() != m_previousCentroids.size()) {
        return true; // Number of clusters changed
    }
    
    // Check if centroids have moved significantly
    float totalMovement = 0.0f;
    for (size_t i = 0; i < m_clusters.size(); i++) {
        float movement = calculateDistance(m_clusters[i].centroid, m_previousCentroids[i]);
        totalMovement += movement;
    }
    
    // Only update if centroids have moved significantly
    return totalMovement > m_clusterStabilityThreshold;
}

void PartitionScene::storePreviousCentroids() {
    m_previousCentroids.clear();
    for (const auto& cluster : m_clusters) {
        m_previousCentroids.push_back(cluster.centroid);
    }
}

// Dynamic clustering methods
void PartitionScene::initializeEntityTracking() {
    m_entityClusterAssignments.clear();
    m_entityDistancesToCentroids.clear();
    m_entityClusterAssignments.resize(m_movingEntities.size(), -1); // -1 means unassigned
    m_entityDistancesToCentroids.resize(m_movingEntities.size(), std::numeric_limits<float>::max());
}

void PartitionScene::ensureTrackingArraysSize() {
    size_t requiredSize = m_movingEntities.size();
    if (m_entityClusterAssignments.size() != requiredSize) {
        m_entityClusterAssignments.resize(requiredSize, -1);
    }
    if (m_entityDistancesToCentroids.size() != requiredSize) {
        m_entityDistancesToCentroids.resize(requiredSize, std::numeric_limits<float>::max());
    }
}

void PartitionScene::updateEntityAssignments() {
    if (m_clusters.empty()) return;
    
    // Ensure tracking arrays are properly sized
    ensureTrackingArraysSize();
    
    m_assignmentsChanged = false;
    
    // Update centroids first based on current assignments
    updateClusterCentroids();
    
    // Check each entity to see if it should change clusters
    for (int i = 0; i < m_movingEntities.size(); i++) {
        if (!m_movingEntities[i].active) continue;
        
        // Only update if entity has moved significantly
        if (hasEntityMovedSignificantly(i)) {
            updateSingleEntityAssignment(i);
        }
    }
    
    // Update colors if assignments changed
    if (m_assignmentsChanged) {
        updateEntityColors(); // Use immediate color updates for cluster changes
        updateQuadtreeVisualization();
    }
}

void PartitionScene::updateSingleEntityAssignment(int entityIndex) {
    if (m_clusters.empty()) return;
    
    // Bounds checking to prevent vector subscript out of range
    if (entityIndex < 0 || entityIndex >= m_movingEntities.size()) return;
    
    // Ensure tracking arrays are properly sized
    ensureTrackingArraysSize();
    
    Vec2 entityPos = m_movingEntities[entityIndex].qtEntity.position;
    float minDistance = std::numeric_limits<float>::max();
    int closestCluster = 0;
    
    // Use quadtree for optimized nearest cluster search
    if (m_quadtree) {
        // First, do a quick check with current cluster assignments
        int currentAssignment = m_entityClusterAssignments[entityIndex];
        if (currentAssignment >= 0 && currentAssignment < m_clusters.size()) {
            float currentDistance = calculateDistance(entityPos, m_clusters[currentAssignment].centroid);
            minDistance = currentDistance;
            closestCluster = currentAssignment;
        }
        
        // Use quadtree to find nearby clusters more efficiently
        float searchRadius = std::min(minDistance * 1.5f, 200.0f); // Search radius based on current distance
        auto nearbyEntities = m_quadtree->query(entityPos, Vec2(searchRadius, searchRadius));
        
        // Check if any nearby entities suggest a closer cluster
        for (const auto& qtEntity : nearbyEntities) {
            // Use helper method to find entity index
            int entityIdx = findEntityIndexByQuadtreeId(qtEntity.id);
            if (entityIdx == -1) continue;
            
            // Find which cluster this entity belongs to
            for (int clusterIdx = 0; clusterIdx < m_kmeansK; clusterIdx++) {
                if (isEntityInCluster(entityIdx, clusterIdx)) {
                    float distanceToCluster = calculateDistance(entityPos, m_clusters[clusterIdx].centroid);
                    if (distanceToCluster < minDistance) {
                        minDistance = distanceToCluster;
                        closestCluster = clusterIdx;
                    }
                    break;
                }
            }
        }
        
        // If we didn't find a closer cluster through quadtree, do a limited brute force check
        if (closestCluster == currentAssignment) {
            // Only check clusters that are reasonably close
            for (int j = 0; j < m_kmeansK; j++) {
                if (j == currentAssignment) continue;
                
                float distance = calculateDistance(entityPos, m_clusters[j].centroid);
                if (distance < minDistance) {
                    minDistance = distance;
                    closestCluster = j;
                }
            }
        }
    } else {
        // Fallback to brute force
        for (int j = 0; j < m_kmeansK; j++) {
            float distance = calculateDistance(entityPos, m_clusters[j].centroid);
            if (distance < minDistance) {
                minDistance = distance;
                closestCluster = j;
            }
        }
    }
    
    // Check if assignment changed
    int currentAssignment = m_entityClusterAssignments[entityIndex];
    if (currentAssignment != closestCluster) {
        // Remove from old cluster
        if (currentAssignment >= 0 && currentAssignment < m_clusters.size()) {
            auto& oldCluster = m_clusters[currentAssignment];
            oldCluster.entityIndices.erase(
                std::remove(oldCluster.entityIndices.begin(), oldCluster.entityIndices.end(), entityIndex),
                oldCluster.entityIndices.end()
            );
        }
        
        // Add to new cluster
        m_clusters[closestCluster].entityIndices.push_back(entityIndex);
        m_entityClusterAssignments[entityIndex] = closestCluster;
        m_entityDistancesToCentroids[entityIndex] = minDistance;
        m_assignmentsChanged = true;
    }
}

bool PartitionScene::hasEntityMovedSignificantly(int entityIndex) {
    if (entityIndex < 0 || entityIndex >= m_movingEntities.size()) return false;
    
    // More responsive movement detection
    if (!m_entitiesMoving) return false;
    
    // Check if entity has moved a significant distance since last check
    // This could be enhanced with position tracking, but for now we'll be more aggressive
    return true; // Always check when entities are moving for maximum responsiveness
}

void PartitionScene::smoothColorTransitions() {
    for (int i = 0; i < m_movingEntities.size(); i++) {
        if (!m_movingEntities[i].active) continue;
        
        // Bounds checking
        if (i >= m_entityClusterAssignments.size()) continue;
        
        int clusterIndex = m_entityClusterAssignments[i];
        if (clusterIndex < 0 || clusterIndex >= m_clusters.size()) continue;
        
        if (auto* entity = m_entityManager->findEntity(m_movingEntities[i].name)) {
            if (auto* sprite = entity->getComponent<SpriteComponent>()) {
                Vec4 targetColor = m_clusters[clusterIndex].color;
                Vec4 currentColor = sprite->getTint();
                
                // Check if we're close enough to target to avoid infinite lerping
                float colorDifference = std::abs(currentColor.x - targetColor.x) + 
                                      std::abs(currentColor.y - targetColor.y) + 
                                      std::abs(currentColor.z - targetColor.z);
                
                if (colorDifference < 0.01f) {
                    // Close enough, set exact target color
                    sprite->setTint(targetColor);
                } else {
                    // Smooth transition over time
                    float lerpFactor = 0.5f; // Faster transition
                    Vec4 newColor = Vec4(
                        currentColor.x + (targetColor.x - currentColor.x) * lerpFactor,
                        currentColor.y + (targetColor.y - currentColor.y) * lerpFactor,
                        currentColor.z + (targetColor.z - currentColor.z) * lerpFactor,
                        targetColor.w
                    );
                    
                    sprite->setTint(newColor);
                }
            }
        }
    }
}

// Quadtree optimization helper methods
int PartitionScene::findEntityIndexByQuadtreeId(int qtEntityId) {
    for (int i = 0; i < m_movingEntities.size(); i++) {
        if (m_movingEntities[i].qtEntity.id == qtEntityId && m_movingEntities[i].active) {
            return i;
        }
    }
    return -1;
}

float PartitionScene::calculateDistanceSquared(const Vec2& a, const Vec2& b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return dx * dx + dy * dy; // Avoid sqrt for distance comparisons
}

bool PartitionScene::isEntityInCluster(int entityIndex, int clusterIndex) {
    if (clusterIndex < 0 || clusterIndex >= m_clusters.size()) return false;
    if (entityIndex < 0 || entityIndex >= m_movingEntities.size()) return false;
    
    const auto& cluster = m_clusters[clusterIndex];
    return std::find(cluster.entityIndices.begin(), cluster.entityIndices.end(), entityIndex) != cluster.entityIndices.end();
}

void PartitionScene::optimizeSpatialQueries() {
    // Pre-calculate cluster bounding boxes for faster spatial queries
    for (int i = 0; i < m_kmeansK; i++) {
        if (m_clusters[i].entityIndices.empty()) continue;
        
        // Calculate cluster bounding box
        Vec2 minPos(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
        Vec2 maxPos(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());
        
        for (int entityIndex : m_clusters[i].entityIndices) {
            if (entityIndex < m_movingEntities.size() && m_movingEntities[entityIndex].active) {
                Vec2 pos = m_movingEntities[entityIndex].qtEntity.position;
                minPos.x = std::min(minPos.x, pos.x);
                minPos.y = std::min(minPos.y, pos.y);
                maxPos.x = std::max(maxPos.x, pos.x);
                maxPos.y = std::max(maxPos.y, pos.y);
            }
        }
        
        // Store bounding box info for future optimizations (if needed)
    }
}


