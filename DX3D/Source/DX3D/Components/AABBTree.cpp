#include <DX3D/Components/AABBTree.h>
#include <algorithm>

using namespace dx3d;

AABBTree::AABBTree(const Vec2& center, const Vec2& size, int leafCapacity, int maxDepth)
    : m_center(center), m_size(size), m_leafCapacity(leafCapacity), m_maxDepth(maxDepth) {
}

AABBTree::~AABBTree() {
    clear();
}

void AABBTree::clear() {
    destroy(m_root); m_root = nullptr; m_allEntities.clear();
}

void AABBTree::destroy(AABBNode* n) {
    if (!n) return;
    destroy(n->left);
    destroy(n->right);
    delete n;
}

bool AABBTree::intersects(const Vec2& c1, const Vec2& h1, const Vec2& c2, const Vec2& h2) {
    return std::abs(c1.x - c2.x) <= (h1.x + h2.x) && std::abs(c1.y - c2.y) <= (h1.y + h2.y);
}

Vec2 AABBTree::minOf(const Vec2& a, const Vec2& b) { return Vec2(std::min(a.x, b.x), std::min(a.y, b.y)); }
Vec2 AABBTree::maxOf(const Vec2& a, const Vec2& b) { return Vec2(std::max(a.x, b.x), std::max(a.y, b.y)); }

void AABBTree::insert(const QuadtreeEntity& e) {
    m_allEntities.push_back(e);
}

void AABBTree::buildFrom(const std::vector<QuadtreeEntity>& entities) {
    clear();
    m_allEntities = entities;
    std::vector<QuadtreeEntity> ents = m_allEntities;
    m_root = buildRecursive(ents, 0);
}

AABBNode* AABBTree::buildRecursive(std::vector<QuadtreeEntity>& ents, int depth) {
    if (ents.empty()) return nullptr;

    AABBNode* node = new AABBNode();
    // Compute bounding box of all ents
    Vec2 minp(1e9f, 1e9f), maxp(-1e9f, -1e9f);
    for (const auto& e : ents) {
        Vec2 half = e.size * 0.5f;
        Vec2 mn = e.position - half;
        Vec2 mx = e.position + half;
        minp = minOf(minp, mn);
        maxp = maxOf(maxp, mx);
    }
    node->center = (minp + maxp) * 0.5f;
    node->halfSize = (maxp - minp) * 0.5f;

    if ((int)ents.size() <= m_leafCapacity || depth >= m_maxDepth) {
        node->entities = ents;
        node->isLeaf = true;
        return node;
    }

    // Split along longest axis at median
    Vec2 ext = node->halfSize * 2.0f;
    bool splitX = ext.x >= ext.y;
    std::sort(ents.begin(), ents.end(), [&](const QuadtreeEntity& a, const QuadtreeEntity& b){
        return splitX ? a.position.x < b.position.x : a.position.y < b.position.y;
    });
    size_t mid = ents.size() / 2;
    std::vector<QuadtreeEntity> left(ents.begin(), ents.begin() + mid);
    std::vector<QuadtreeEntity> right(ents.begin() + mid, ents.end());
    node->left = buildRecursive(left, depth + 1);
    node->right = buildRecursive(right, depth + 1);
    node->isLeaf = false;
    return node;
}

std::vector<QuadtreeEntity> AABBTree::query(const Vec2& center, const Vec2& halfSize) const {
    std::vector<QuadtreeEntity> out; out.reserve(64);
    queryRecursive(m_root, center, halfSize, out);
    return out;
}

void AABBTree::queryRecursive(AABBNode* n, const Vec2& center, const Vec2& halfSize, std::vector<QuadtreeEntity>& out) const {
    if (!n) return;
    if (!intersects(n->center, n->halfSize, center, halfSize)) return;
    if (n->isLeaf) {
        for (const auto& e : n->entities) out.push_back(e);
        return;
    }
    queryRecursive(n->left, center, halfSize, out);
    queryRecursive(n->right, center, halfSize, out);
}

void AABBTree::getAllNodes(std::vector<AABBNode*>& out) const {
    collectNodes(m_root, out);
}

void AABBTree::collectNodes(AABBNode* n, std::vector<AABBNode*>& out) const {
    if (!n) return;
    out.push_back(n);
    collectNodes(n->left, out);
    collectNodes(n->right, out);
}


