#include <DX3D/Components/Quadtree.h>
#include <algorithm>

using namespace dx3d;

Quadtree::Quadtree(const Vec2& center, const Vec2& size, int maxEntities, int maxDepth)
    : m_center(center), m_size(size), m_maxEntities(maxEntities), m_maxDepth(maxDepth), m_depth(0) {
}

void Quadtree::insert(const QuadtreeEntity& entity) {
    // Check if entity is within bounds
    if (!contains(entity.position)) {
        return;
    }

    // If this is a leaf node and we haven't exceeded capacity
    if (isLeaf() && m_entities.size() < m_maxEntities) {
        m_entities.push_back(entity);
        return;
    }

    // If we haven't subdivided yet, do so
    if (isLeaf() && m_depth < m_maxDepth) {
        subdivide();
    }

    // If we have children, try to insert into them
    if (!isLeaf()) {
        int quadrant = getQuadrant(entity.position);
        if (quadrant != -1) {
            m_children[quadrant]->insert(entity);
        }
        else {
            // Entity spans multiple quadrants, keep it in this node
            m_entities.push_back(entity);
        }
    }
    else {
        // We're at max depth, just add to this node
        m_entities.push_back(entity);
    }
}

std::vector<QuadtreeEntity> Quadtree::query(const Vec2& center, const Vec2& size) const {
    std::vector<QuadtreeEntity> result;

    if (!intersects(center, size)) {
        return result;
    }

    // Add entities from this node that intersect with the query area
    for (const auto& entity : m_entities) {
        if (entity.position.x >= center.x - size.x * 0.5f &&
            entity.position.x <= center.x + size.x * 0.5f &&
            entity.position.y >= center.y - size.y * 0.5f &&
            entity.position.y <= center.y + size.y * 0.5f) {
            result.push_back(entity);
        }
    }

    // Query children
    if (!isLeaf()) {
        for (int i = 0; i < 4; i++) {
            auto childResults = m_children[i]->query(center, size);
            result.insert(result.end(), childResults.begin(), childResults.end());
        }
    }

    return result;
}

void Quadtree::clear() {
    m_entities.clear();
    for (int i = 0; i < 4; i++) {
        if (m_children[i]) {
            m_children[i]->clear();
        }
    }
}

void Quadtree::getAllNodes(std::vector<Quadtree*>& nodes) const {
    nodes.push_back(const_cast<Quadtree*>(this));

    if (!isLeaf()) {
        for (int i = 0; i < 4; i++) {
            m_children[i]->getAllNodes(nodes);
        }
    }
}

bool Quadtree::contains(const Vec2& point) const {
    return point.x >= m_center.x - m_size.x * 0.5f &&
        point.x <= m_center.x + m_size.x * 0.5f &&
        point.y >= m_center.y - m_size.y * 0.5f &&
        point.y <= m_center.y + m_size.y * 0.5f;
}

bool Quadtree::intersects(const Vec2& center, const Vec2& size) const {
    Vec2 halfSize = size * 0.5f;
    Vec2 halfThisSize = m_size * 0.5f;

    return !(center.x + halfSize.x < m_center.x - halfThisSize.x ||
        center.x - halfSize.x > m_center.x + halfThisSize.x ||
        center.y + halfSize.y < m_center.y - halfThisSize.y ||
        center.y - halfSize.y > m_center.y + halfThisSize.y);
}

void Quadtree::subdivide() {
    Vec2 halfSize = m_size * 0.5f;
    Vec2 quarterSize = m_size * 0.25f;

    // Create four children: NW, NE, SW, SE
    m_children[0] = std::make_unique<Quadtree>(Vec2(m_center.x - quarterSize.x, m_center.y - quarterSize.y), halfSize, m_maxEntities, m_maxDepth);
    m_children[1] = std::make_unique<Quadtree>(Vec2(m_center.x + quarterSize.x, m_center.y - quarterSize.y), halfSize, m_maxEntities, m_maxDepth);
    m_children[2] = std::make_unique<Quadtree>(Vec2(m_center.x - quarterSize.x, m_center.y + quarterSize.y), halfSize, m_maxEntities, m_maxDepth);
    m_children[3] = std::make_unique<Quadtree>(Vec2(m_center.x + quarterSize.x, m_center.y + quarterSize.y), halfSize, m_maxEntities, m_maxDepth);

    // Set depth for children
    for (int i = 0; i < 4; i++) {
        m_children[i]->m_depth = m_depth + 1;
    }

    // Redistribute existing entities
    std::vector<QuadtreeEntity> entitiesToRedistribute = m_entities;
    m_entities.clear();

    for (const auto& entity : entitiesToRedistribute) {
        int quadrant = getQuadrant(entity.position);
        if (quadrant != -1) {
            m_children[quadrant]->insert(entity);
        }
        else {
            m_entities.push_back(entity);
        }
    }
}

int Quadtree::getQuadrant(const Vec2& position) const {
    if (position.x < m_center.x && position.y < m_center.y) {
        return 0; // NW
    }
    else if (position.x >= m_center.x && position.y < m_center.y) {
        return 1; // NE
    }
    else if (position.x < m_center.x && position.y >= m_center.y) {
        return 2; // SW
    }
    else {
        return 3; // SE
    }
}