#include <DX3D/Components/Octree.h>
#include <algorithm>

using namespace dx3d;

Octree::Octree(const Vec3& center, const Vec3& size, int maxEntities, int maxDepth)
    : m_center(center), m_size(size), m_maxEntities(maxEntities), m_maxDepth(maxDepth), m_depth(0) {
}

void Octree::insert(const OctreeEntity& entity) {
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
        int octant = getOctant(entity.position);
        if (octant != -1) {
            m_children[octant]->insert(entity);
        }
        else {
            // Entity spans multiple octants, keep it in this node
            m_entities.push_back(entity);
        }
    }
    else {
        // We're at max depth, just add to this node
        m_entities.push_back(entity);
    }
}

std::vector<OctreeEntity> Octree::query(const Vec3& center, const Vec3& size) const {
    std::vector<OctreeEntity> result;
    
    // Check if query area intersects this node
    if (!intersects(center, size)) {
        return result;
    }
    
    // Add entities from this node
    for (const auto& entity : m_entities) {
        if (intersects(center, size)) {
            result.push_back(entity);
        }
    }
    
    // Query children if they exist
    if (!isLeaf()) {
        for (int i = 0; i < 8; i++) {
            if (m_children[i]) {
                auto childResults = m_children[i]->query(center, size);
                result.insert(result.end(), childResults.begin(), childResults.end());
            }
        }
    }
    
    return result;
}

void Octree::clear() {
    m_entities.clear();
    for (int i = 0; i < 8; i++) {
        m_children[i].reset();
    }
}

void Octree::getAllNodes(std::vector<Octree*>& nodes) const {
    nodes.push_back(const_cast<Octree*>(this));
    
    if (!isLeaf()) {
        for (int i = 0; i < 8; i++) {
            if (m_children[i]) {
                m_children[i]->getAllNodes(nodes);
            }
        }
    }
}

bool Octree::contains(const Vec3& point) const {
    Vec3 halfSize = m_size * 0.5f;
    return point.x >= m_center.x - halfSize.x && point.x <= m_center.x + halfSize.x &&
           point.y >= m_center.y - halfSize.y && point.y <= m_center.y + halfSize.y &&
           point.z >= m_center.z - halfSize.z && point.z <= m_center.z + halfSize.z;
}

bool Octree::intersects(const Vec3& center, const Vec3& size) const {
    Vec3 halfSize1 = m_size * 0.5f;
    Vec3 halfSize2 = size * 0.5f;
    
    return m_center.x - halfSize1.x <= center.x + halfSize2.x &&
           m_center.x + halfSize1.x >= center.x - halfSize2.x &&
           m_center.y - halfSize1.y <= center.y + halfSize2.y &&
           m_center.y + halfSize1.y >= center.y - halfSize2.y &&
           m_center.z - halfSize1.z <= center.z + halfSize2.z &&
           m_center.z + halfSize1.z >= center.z - halfSize2.z;
}

void Octree::subdivide() {
    Vec3 halfSize = m_size * 0.5f;
    Vec3 quarterSize = halfSize * 0.5f;
    
    // Create 8 children for each octant
    m_children[0] = std::make_unique<Octree>(Vec3(m_center.x - quarterSize.x, m_center.y + quarterSize.y, m_center.z + quarterSize.z), halfSize, m_maxEntities, m_maxDepth);
    m_children[1] = std::make_unique<Octree>(Vec3(m_center.x + quarterSize.x, m_center.y + quarterSize.y, m_center.z + quarterSize.z), halfSize, m_maxEntities, m_maxDepth);
    m_children[2] = std::make_unique<Octree>(Vec3(m_center.x - quarterSize.x, m_center.y - quarterSize.y, m_center.z + quarterSize.z), halfSize, m_maxEntities, m_maxDepth);
    m_children[3] = std::make_unique<Octree>(Vec3(m_center.x + quarterSize.x, m_center.y - quarterSize.y, m_center.z + quarterSize.z), halfSize, m_maxEntities, m_maxDepth);
    m_children[4] = std::make_unique<Octree>(Vec3(m_center.x - quarterSize.x, m_center.y + quarterSize.y, m_center.z - quarterSize.z), halfSize, m_maxEntities, m_maxDepth);
    m_children[5] = std::make_unique<Octree>(Vec3(m_center.x + quarterSize.x, m_center.y + quarterSize.y, m_center.z - quarterSize.z), halfSize, m_maxEntities, m_maxDepth);
    m_children[6] = std::make_unique<Octree>(Vec3(m_center.x - quarterSize.x, m_center.y - quarterSize.y, m_center.z - quarterSize.z), halfSize, m_maxEntities, m_maxDepth);
    m_children[7] = std::make_unique<Octree>(Vec3(m_center.x + quarterSize.x, m_center.y - quarterSize.y, m_center.z - quarterSize.z), halfSize, m_maxEntities, m_maxDepth);
    
    // Set depth for children
    for (int i = 0; i < 8; i++) {
        m_children[i]->m_depth = m_depth + 1;
    }
}

int Octree::getOctant(const Vec3& position) const {
    Vec3 halfSize = m_size * 0.5f;
    
    // Determine which octant the position falls into
    bool left = position.x < m_center.x;
    bool top = position.y > m_center.y;
    bool front = position.z > m_center.z;
    
    if (left && top && front) return 0;
    if (!left && top && front) return 1;
    if (left && !top && front) return 2;
    if (!left && !top && front) return 3;
    if (left && top && !front) return 4;
    if (!left && top && !front) return 5;
    if (left && !top && !front) return 6;
    if (!left && !top && !front) return 7;
    
    return -1; // Position is exactly on a boundary
}

bool Octree::hasEntitiesInSubtree() const {
    // Check if this node has entities
    if (!m_entities.empty()) {
        return true;
    }
    
    // Check if any children have entities
    if (!isLeaf()) {
        for (int i = 0; i < 8; i++) {
            if (m_children[i] && m_children[i]->hasEntitiesInSubtree()) {
                return true;
            }
        }
    }
    
    return false;
}