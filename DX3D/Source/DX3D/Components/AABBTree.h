#pragma once
#include <DX3D/Math/Geometry.h>
#include <DX3D/Components/Quadtree.h> // for QuadtreeEntity definition
#include <vector>

namespace dx3d {

    struct AABBNode {
        Vec2 center;     // center of the node AABB
        Vec2 halfSize;   // half-extent (width/2, height/2)
        std::vector<QuadtreeEntity> entities; // payload at leaves
        AABBNode* left = nullptr;
        AABBNode* right = nullptr;
        bool isLeaf = false;
    };

    class AABBTree {
    public:
        AABBTree(const Vec2& center, const Vec2& size, int leafCapacity = 8, int maxDepth = 16);
        ~AABBTree();

        void clear();
        void insert(const QuadtreeEntity& e);
        void buildFrom(const std::vector<QuadtreeEntity>& entities);
        std::vector<QuadtreeEntity> query(const Vec2& center, const Vec2& halfSize) const;
        void getAllNodes(std::vector<AABBNode*>& out) const;

    private:
        AABBNode* m_root = nullptr;
        Vec2 m_center;
        Vec2 m_size;
        int m_leafCapacity;
        int m_maxDepth;
        std::vector<QuadtreeEntity> m_allEntities;

        static bool intersects(const Vec2& c1, const Vec2& h1, const Vec2& c2, const Vec2& h2);
        static Vec2 minOf(const Vec2& a, const Vec2& b);
        static Vec2 maxOf(const Vec2& a, const Vec2& b);

        void destroy(AABBNode* n);
        AABBNode* buildRecursive(std::vector<QuadtreeEntity>& ents, int depth);
        void queryRecursive(AABBNode* n, const Vec2& center, const Vec2& halfSize, std::vector<QuadtreeEntity>& out) const;
        void collectNodes(AABBNode* n, std::vector<AABBNode*>& out) const;
    };
}


