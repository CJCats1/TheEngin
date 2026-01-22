// AdvancedSlicingSystem.h - Advanced block slicing for physics tetris
#pragma once
#include <vector>
#include <memory>
#include <DX3D/Math/Geometry.h>
#include <DX3D/Core/Entity.h>
#include <DX3D/Components/TetrisPhysicsComponent.h>
#include <DX3D/Graphics/SpriteComponent.h>

namespace dx3d {

    struct SliceData {
        std::vector<Vec2> vertices;
        Vec4 color;
        float mass;
        Vec2 centerOfMass;
    };

    class AdvancedSlicingSystem {
    public:
        // Main slicing function - cuts all blocks intersecting with the line
        static std::vector<SliceData> sliceBlocksAtLine(
            const std::vector<Entity*>& blocks,
            float lineY,
            float lineThickness = 32.0f
        ) {
            std::vector<SliceData> results;

            for (Entity* block : blocks) {
                auto slicedParts = sliceBlock(block, lineY, lineThickness);
                results.insert(results.end(), slicedParts.begin(), slicedParts.end());
            }

            return results;
        }

    private:
        // Slice a single block into multiple pieces
        static std::vector<SliceData> sliceBlock(Entity* block, float lineY, float thickness) {
            std::vector<SliceData> results;

            auto* physics = block->getComponent<TetrisPhysicsComponent>();
            auto* sprite = block->getComponent<SpriteComponent>();

            if (!physics || !sprite) return results;

            Vec2 blockPos = physics->getPosition();
            Vec2 blockSize(32.0f, 32.0f); // Assuming 32x32 blocks
            Vec4 blockColor = sprite->getTint();

            // Get block bounds
            float blockTop = blockPos.y - blockSize.y / 2.0f;
            float blockBottom = blockPos.y + blockSize.y / 2.0f;
            float lineTop = lineY - thickness / 2.0f;
            float lineBottom = lineY + thickness / 2.0f;

            // Check if block intersects with cutting line
            if (blockBottom < lineTop || blockTop > lineBottom) {
                // No intersection - keep original block
                SliceData originalBlock;
                originalBlock.vertices = getBlockVertices(blockPos, blockSize);
                originalBlock.color = blockColor;
                originalBlock.mass = 1.0f;
                originalBlock.centerOfMass = blockPos;
                results.push_back(originalBlock);
                return results;
            }

            // Block intersects - create sliced pieces

            // Upper piece (if any)
            if (blockTop < lineTop) {
                SliceData upperPiece;
                float upperHeight = lineTop - blockTop;
                Vec2 upperPos(blockPos.x, blockTop + upperHeight / 2.0f);
                Vec2 upperSize(blockSize.x, upperHeight);

                upperPiece.vertices = getBlockVertices(upperPos, upperSize);
                upperPiece.color = blockColor;
                upperPiece.mass = (upperHeight / blockSize.y); // Mass proportional to size
                upperPiece.centerOfMass = upperPos;
                results.push_back(upperPiece);
            }

            // Lower piece (if any)
            if (blockBottom > lineBottom) {
                SliceData lowerPiece;
                float lowerHeight = blockBottom - lineBottom;
                Vec2 lowerPos(blockPos.x, lineBottom + lowerHeight / 2.0f);
                Vec2 lowerSize(blockSize.x, lowerHeight);

                lowerPiece.vertices = getBlockVertices(lowerPos, lowerSize);
                lowerPiece.color = blockColor;
                lowerPiece.mass = (lowerHeight / blockSize.y); // Mass proportional to size
                lowerPiece.centerOfMass = lowerPos;
                results.push_back(lowerPiece);
            }

            // Middle piece gets removed (represents the sliced away part)

            return results;
        }

        // Generate vertices for a rectangular block
        static std::vector<Vec2> getBlockVertices(Vec2 center, Vec2 size) {
            std::vector<Vec2> vertices;
            float halfWidth = size.x / 2.0f;
            float halfHeight = size.y / 2.0f;

            vertices.push_back(Vec2(center.x - halfWidth, center.y - halfHeight)); // Top-left
            vertices.push_back(Vec2(center.x + halfWidth, center.y - halfHeight)); // Top-right
            vertices.push_back(Vec2(center.x + halfWidth, center.y + halfHeight)); // Bottom-right
            vertices.push_back(Vec2(center.x - halfWidth, center.y + halfHeight)); // Bottom-left

            return vertices;
        }

        // Calculate polygon area using shoelace formula
        static float calculatePolygonArea(const std::vector<Vec2>& vertices) {
            if (vertices.size() < 3) return 0.0f;

            float area = 0.0f;
            size_t n = vertices.size();

            for (size_t i = 0; i < n; i++) {
                size_t j = (i + 1) % n;
                area += vertices[i].x * vertices[j].y;
                area -= vertices[j].x * vertices[i].y;
            }

            return abs(area) / 2.0f;
        }

        // Calculate center of mass for a polygon
        static Vec2 calculateCenterOfMass(const std::vector<Vec2>& vertices) {
            if (vertices.empty()) return Vec2(0.0f, 0.0f);

            Vec2 center(0.0f, 0.0f);
            for (const Vec2& vertex : vertices) {
                center += vertex;
            }

            return center / static_cast<float>(vertices.size());
        }
    };


        // Enhanced line clearing with advanced slicing
       
}