#pragma once
#include <DX3D/Math/Geometry.h>
#include <DX3D/Components/Quadtree.h>
#include <vector>

namespace dx3d {

    struct KDNode {
        Vec2 center;
        Vec2 halfSize;
        std::vector<QuadtreeEntity> entities; // at leaves
        KDNode* left = nullptr;
        KDNode* right = nullptr;
        bool isLeaf = false;
        int axis = 0; // 0=x,1=y
        float split = 0.0f; // split coordinate along axis for non-leaf
    };

    class KDTree {
    public:
        KDTree(const Vec2& center, const Vec2& size, int leafCapacity = 8, int maxDepth = 16);
        ~KDTree();

        void clear();
        void buildFrom(const std::vector<QuadtreeEntity>& entities);
        void getAllNodes(std::vector<KDNode*>& out) const;

    private:
        KDNode* m_root = nullptr;
        Vec2 m_center;
        Vec2 m_size;
        int m_leafCapacity;
        int m_maxDepth;

        static Vec2 minOf(const Vec2& a, const Vec2& b);
        static Vec2 maxOf(const Vec2& a, const Vec2& b);

        void destroy(KDNode* n);
        KDNode* buildRecursive(std::vector<QuadtreeEntity>& ents, const Vec2& center, const Vec2& halfSize, int depth, int axis);
        void collectNodes(KDNode* n, std::vector<KDNode*>& out) const;
    };
}


