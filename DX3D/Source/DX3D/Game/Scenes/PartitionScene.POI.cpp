#include <DX3D/Game/Scenes/PartitionScene.h>
#include <DX3D/Graphics/LineRenderer.h>
#include <random>
#include <algorithm>
#include <cmath>

using namespace dx3d;

void PartitionScene::addPointOfInterest(const Vec2& position, const std::string& name) {
    PointOfInterest poi;
    poi.position = position;
    poi.name = name.empty() ? "POI_" + std::to_string(m_pointsOfInterest.size()) : name;
    poi.color = Vec4(1.0f, 1.0f, 0.0f, 0.8f);
    poi.attractionRadius = 100.0f;
    poi.attractionStrength = 1.0f;
    poi.active = true;
    
    m_pointsOfInterest.push_back(poi);
    updateQuadtreeVisualization();
}

void PartitionScene::removePointOfInterest(int index) {
    if (index >= 0 && index < m_pointsOfInterest.size()) {
        m_pointsOfInterest.erase(m_pointsOfInterest.begin() + index);
        updateQuadtreeVisualization();
    }
}

void PartitionScene::clearAllPOIs() {
    m_pointsOfInterest.clear();
    for (auto& entity : m_movingEntities) {
        entity.currentPOI = -1;
        entity.poiSwitchTimer = 0.0f;
    }
    updateQuadtreeVisualization();
}

void PartitionScene::updatePOIAttraction() {
    for (auto& movingEntity : m_movingEntities) {
        if (!movingEntity.active) continue;
        movingEntity.velocity.x *= 0.999f;
        movingEntity.velocity.y *= 0.999f;
        movingEntity.poiSwitchTimer += 0.016f;
        if (movingEntity.poiSwitchTimer >= m_poiSwitchInterval || 
            movingEntity.currentPOI < 0 || 
            movingEntity.currentPOI >= m_pointsOfInterest.size() ||
            !m_pointsOfInterest[movingEntity.currentPOI].active) {
            selectPOIForEntity(movingEntity);
            movingEntity.poiSwitchTimer = 0.0f;
        }
        if (movingEntity.currentPOI >= 0 && movingEntity.currentPOI < m_pointsOfInterest.size()) {
            const auto& poi = m_pointsOfInterest[movingEntity.currentPOI];
            if (poi.active) {
                Vec2 attractionForce = calculatePOIAttractionForce(movingEntity, poi);
                movingEntity.velocity += attractionForce;
            }
        }
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> randomVel(-0.5f, 0.5f);
        movingEntity.velocity.x += randomVel(gen);
        movingEntity.velocity.y += randomVel(gen);
    }
}

void PartitionScene::selectPOIForEntity(MovingEntity& entity) {
    if (m_pointsOfInterest.empty()) {
        entity.currentPOI = -1;
        return;
    }
    Vec2 entityPos = entity.qtEntity.position;
    float bestScore = -1.0f;
    int bestPOI = -1;
    for (int i = 0; i < m_pointsOfInterest.size(); i++) {
        const auto& poi = m_pointsOfInterest[i];
        if (!poi.active) continue;
        float distance = calculateDistance(entityPos, poi.position);
        if (distance <= poi.attractionRadius) {
            float score = poi.attractionStrength / (distance + 1.0f);
            if (score > bestScore) { bestScore = score; bestPOI = i; }
        }
    }
    if (bestPOI == -1) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, m_pointsOfInterest.size() - 1);
        bestPOI = dis(gen);
    }
    entity.currentPOI = bestPOI;
}

Vec2 PartitionScene::calculatePOIAttractionForce(const MovingEntity& entity, const PointOfInterest& poi) {
    Vec2 entityPos = entity.qtEntity.position;
    Vec2 direction = poi.position - entityPos;
    float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    if (distance < 15.0f) return Vec2(0.0f, 0.0f);
    direction.x /= distance;
    direction.y /= distance;
    float minDistance = 15.0f;
    float effectiveDistance = std::max(distance, minDistance);
    float baseForceStrength = m_poiAttractionStrength * poi.attractionStrength * entity.poiAttractionStrength * 1000.0f;
    float force = baseForceStrength / (effectiveDistance * effectiveDistance);
    if (distance > poi.attractionRadius) {
        float excessDistance = distance - poi.attractionRadius;
        float falloffFactor = std::exp(-excessDistance / 50.0f);
        force *= falloffFactor;
    }
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> randomFactor(0.7f, 1.3f);
    force *= randomFactor(gen);
    float maxForce = 2.0f;
    force = std::min(force, maxForce);
    return Vec2(direction.x * force, direction.y * force);
}

void PartitionScene::createDefaultPOIs() {
    addPointOfInterest(Vec2(-200.0f, -150.0f), "Base Camp");
    addPointOfInterest(Vec2(200.0f, 150.0f), "Resource Point");
    addPointOfInterest(Vec2(0.0f, 200.0f), "Objective Alpha");
    addPointOfInterest(Vec2(-150.0f, 100.0f), "Objective Beta");
    addPointOfInterest(Vec2(150.0f, -100.0f), "Objective Gamma");
}

void PartitionScene::updatePOIStatus() {
    if (m_poiStatusText) {
        std::wstring statusText = m_poiSystemEnabled ? L"POI System: Enabled" : L"POI System: Disabled";
        m_poiStatusText->setText(statusText);
    }
    if (m_poiCountText) {
        int activePOICount = 0;
        for (const auto& poi : m_pointsOfInterest) {
            if (poi.active) activePOICount++;
        }
        std::wstring countText = L"POIs: " + std::to_wstring(activePOICount);
        m_poiCountText->setText(countText);
    }
    if (m_poiStrengthText) {
        std::wstring strengthText = L"POI Strength: " + std::to_wstring(static_cast<int>(m_poiAttractionStrength * 10) / 10.0f);
        m_poiStrengthText->setText(strengthText);
    }
    if (m_entitySpeedText) {
        std::wstring speedText = L"Entity Speed: " + std::to_wstring(static_cast<int>(m_entitySpeedMultiplier * 10) / 10.0f) + L"x";
        m_entitySpeedText->setText(speedText);
    }
}


