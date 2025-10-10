#pragma once
#include <DX3D/Math/Geometry.h>
#include <vector>
#include <memory>

namespace dx3d {

    struct OctreeEntity {
        Vec3 position;
        Vec3 size;
        int id;
    };

    class Octree {
    public:
        Octree(const Vec3& center, const Vec3& size, int maxEntities = 4, int maxDepth = 5);
        ~Octree() = default;

        // Insert an entity into the octree
        void insert(const OctreeEntity& entity);

        // Query entities in a given area
        std::vector<OctreeEntity> query(const Vec3& center, const Vec3& size) const;

        // Clear all entities
        void clear();

        // Get bounds for visualization
        Vec3 getCenter() const { return m_center; }
        Vec3 getSize() const { return m_size; }
        bool isLeaf() const { return m_children[0] == nullptr; }

        // Get all nodes for visualization
        void getAllNodes(std::vector<Octree*>& nodes) const;

        // Get entities in this node
        const std::vector<OctreeEntity>& getEntities() const { return m_entities; }

    private:
        Vec3 m_center;
        Vec3 m_size;
        int m_maxEntities;
        int m_maxDepth;
        int m_depth;

        std::vector<OctreeEntity> m_entities;
        std::unique_ptr<Octree> m_children[8]; // 8 octants

        // Helper methods
        bool contains(const Vec3& point) const;
        bool intersects(const Vec3& center, const Vec3& size) const;
        void subdivide();
        int getOctant(const Vec3& position) const;
    };

}
