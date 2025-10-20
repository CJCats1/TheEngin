#pragma once
#include <vector>
#include <array>
#include <memory>

namespace dx3d
{
    // LiquidFun-inspired optimized particle system
    class OptimizedParticleSystem
    {
    public:
        // Structure of Arrays (SoA) for better cache locality and SIMD
        struct ParticleData
        {
            std::vector<float> positions_x;    // x coordinates
            std::vector<float> positions_y;    // y coordinates  
            std::vector<float> velocities_x;   // x velocities
            std::vector<float> velocities_y;   // y velocities
            std::vector<float> radii;           // particle radii
            std::vector<uint32_t> colors;       // packed RGBA colors
            std::vector<uint16_t> entity_ids;  // lightweight entity references
            
            size_t count = 0;
            size_t capacity = 0;
        };

        // Spatial grid for O(1) neighbor lookup
        struct SpatialGrid
        {
            int grid_width;
            int grid_height;
            float cell_size;
            Vec2 world_min;
            
            // Fixed-size buckets for better cache performance
            std::vector<std::vector<uint16_t>> cells;
            std::vector<uint16_t> temp_neighbors; // reusable temp storage
        };

        // Contact caching to avoid redundant collision checks
        struct ContactCache
        {
            struct Contact
            {
                uint16_t particle_a;
                uint16_t particle_b;
                float separation;
                Vec2 normal;
                float time_to_live;
            };
            
            std::vector<Contact> contacts;
            std::vector<bool> contact_exists; // bitfield for O(1) lookup
        };

        // Island-based simulation for independent particle groups
        struct ParticleIsland
        {
            std::vector<uint16_t> particle_indices;
            bool is_sleeping = false;
            float sleep_timer = 0.0f;
        };

    private:
        ParticleData m_particles;
        SpatialGrid m_spatial_grid;
        ContactCache m_contact_cache;
        std::vector<ParticleIsland> m_islands;
        
        // Performance counters
        uint32_t m_collision_checks = 0;
        uint32_t m_contacts_found = 0;
        float m_average_neighbors = 0.0f;
    };
}
