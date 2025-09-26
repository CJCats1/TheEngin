#include <DX3D/Components/KDTree.h>
#include <algorithm>

using namespace dx3d;

KDTree::KDTree(const Vec2& center, const Vec2& size, int leafCapacity, int maxDepth)
    : m_center(center), m_size(size), m_leafCapacity(leafCapacity), m_maxDepth(maxDepth) {}

KDTree::~KDTree() { clear(); }

void KDTree::clear() { destroy(m_root); m_root = nullptr; }

void KDTree::destroy(KDNode* n) {
    if (!n) return;
    destroy(n->left); destroy(n->right); delete n;
}

Vec2 KDTree::minOf(const Vec2& a, const Vec2& b) { return Vec2(std::min(a.x, b.x), std::min(a.y, b.y)); }
Vec2 KDTree::maxOf(const Vec2& a, const Vec2& b) { return Vec2(std::max(a.x, b.x), std::max(a.y, b.y)); }

void KDTree::buildFrom(const std::vector<QuadtreeEntity>& entities) {
    clear();
    std::vector<QuadtreeEntity> ents = entities;
    m_root = buildRecursive(ents, m_center, m_size * 0.5f, 0, 0);
}

KDNode* KDTree::buildRecursive(std::vector<QuadtreeEntity>& ents, const Vec2& center, const Vec2& halfSize, int depth, int axis) {
    if (ents.empty()) return nullptr;
    KDNode* node = new KDNode();
    node->center = center;
    node->halfSize = halfSize;
    node->axis = axis;

    if ((int)ents.size() <= m_leafCapacity || depth >= m_maxDepth) {
        node->entities = ents; node->isLeaf = true; return node;
    }

    // Split space at median along current axis
    std::sort(ents.begin(), ents.end(), [&](const QuadtreeEntity& a, const QuadtreeEntity& b){
        return axis == 0 ? a.position.x < b.position.x : a.position.y < b.position.y;
    });
    size_t mid = ents.size()/2;
    float splitCoord = axis == 0 ? ents[mid].position.x : ents[mid].position.y;

    // Partition entities into left/right by split coordinate
    std::vector<QuadtreeEntity> left, right;
    left.reserve(mid); right.reserve(ents.size()-mid);
    for (const auto& e : ents) {
        float c = axis == 0 ? e.position.x : e.position.y;
        if (c <= splitCoord) left.push_back(e); else right.push_back(e);
    }

    // Child bounding boxes (exact from parent bounds)
    Vec2 minP = center - halfSize;
    Vec2 maxP = center + halfSize;
    Vec2 leftMin = minP, leftMax = maxP;
    Vec2 rightMin = minP, rightMax = maxP;
    if (axis == 0) { // X split
        leftMax.x = splitCoord;
        rightMin.x = splitCoord;
    } else { // Y split
        leftMax.y = splitCoord;
        rightMin.y = splitCoord;
    }
    Vec2 leftCenter = (leftMin + leftMax) * 0.5f;
    Vec2 rightCenter = (rightMin + rightMax) * 0.5f;
    Vec2 leftHalf = (leftMax - leftMin) * 0.5f;
    Vec2 rightHalf = (rightMax - rightMin) * 0.5f;

    // Handle degenerate splits (all points equal on axis) by switching axis or forcing a midpoint
    if ((axis == 0 && leftHalf.x <= 1e-4f) || (axis == 1 && leftHalf.y <= 1e-4f)) {
        // try alternate axis once
        int altAxis = 1 - axis;
        std::sort(ents.begin(), ents.end(), [&](const QuadtreeEntity& a, const QuadtreeEntity& b){
            return altAxis == 0 ? a.position.x < b.position.x : a.position.y < b.position.y;
        });
        size_t mid2 = ents.size()/2;
        float split2 = altAxis == 0 ? ents[mid2].position.x : ents[mid2].position.y;
        axis = altAxis; // switch
        splitCoord = split2;
        // recompute child boxes
        minP = center - halfSize; maxP = center + halfSize;
        leftMin = minP; leftMax = maxP; rightMin = minP; rightMax = maxP;
        if (axis == 0) { leftMax.x = splitCoord; rightMin.x = splitCoord; }
        else { leftMax.y = splitCoord; rightMin.y = splitCoord; }
        leftCenter = (leftMin + leftMax) * 0.5f; rightCenter = (rightMin + rightMax) * 0.5f;
        leftHalf = (leftMax - leftMin) * 0.5f; rightHalf = (rightMax - rightMin) * 0.5f;
    }

    node->split = splitCoord;

    node->left = buildRecursive(left, leftCenter, leftHalf, depth+1, 1-axis);
    node->right = buildRecursive(right, rightCenter, rightHalf, depth+1, 1-axis);
    node->isLeaf = false;
    return node;
}

void KDTree::getAllNodes(std::vector<KDNode*>& out) const {
    collectNodes(m_root, out);
}

void KDTree::collectNodes(KDNode* n, std::vector<KDNode*>& out) const {
    if (!n) return; out.push_back(n);
    collectNodes(n->left, out); collectNodes(n->right, out);
}


