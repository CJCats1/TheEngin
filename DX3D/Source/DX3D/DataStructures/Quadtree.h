#pragma once
#include <DX3D/Math/Geometry.h>
#include <vector>
#include <memory>

namespace dx3d {

    struct QuadtreeEntity {
        Vec2 position;
        Vec2 size;
        int id;
    };

    class Quadtree {
    public:
        Quadtree(const Vec2& center, const Vec2& size, int maxEntities = 4, int maxDepth = 5);
        ~Quadtree() = default;

        // Insert an entity into the quadtree
        void insert(const QuadtreeEntity& entity);

        // Query entities in a given area
        std::vector<QuadtreeEntity> query(const Vec2& center, const Vec2& size) const;

        // Clear all entities
        void clear();

        // Get bounds for visualization
        Vec2 getCenter() const { return m_center; }
        Vec2 getSize() const { return m_size; }
        bool isLeaf() const { return m_children[0] == nullptr; }

        // Get all nodes for visualization
        void getAllNodes(std::vector<Quadtree*>& nodes) const;

        // Get entities in this node
        const std::vector<QuadtreeEntity>& getEntities() const { return m_entities; }

    private:
        Vec2 m_center;
        Vec2 m_size;
        int m_maxEntities;
        int m_maxDepth;
        int m_depth;

        std::vector<QuadtreeEntity> m_entities;
        std::unique_ptr<Quadtree> m_children[4]; // NW, NE, SW, SE

        // Helper methods
        bool contains(const Vec2& point) const;
        bool intersects(const Vec2& center, const Vec2& size) const;
        void subdivide();
        int getQuadrant(const Vec2& position) const;
    };

}