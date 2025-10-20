#include "OptimizedParticleSystem.h"
#include <algorithm>
#include <cmath>

namespace dx3d
{
    void OptimizedParticleSystem::updateSpatialGrid()
    {
        // Clear all cells
        for (auto& cell : m_spatial_grid.cells)
        {
            cell.clear();
        }
        
        // Rebuild grid with current particle positions
        for (uint16_t i = 0; i < m_particles.count; ++i)
        {
            float x = m_particles.positions_x[i];
            float y = m_particles.positions_y[i];
            
            // Convert world position to grid coordinates
            int grid_x = static_cast<int>((x - m_spatial_grid.world_min.x) / m_spatial_grid.cell_size);
            int grid_y = static_cast<int>((y - m_spatial_grid.world_min.y) / m_spatial_grid.cell_size);
            
            // Clamp to grid bounds
            grid_x = std::max(0, std::min(grid_x, m_spatial_grid.grid_width - 1));
            grid_y = std::max(0, std::min(grid_y, m_spatial_grid.grid_height - 1));
            
            // Add particle to grid cell
            int cell_index = grid_y * m_spatial_grid.grid_width + grid_x;
            m_spatial_grid.cells[cell_index].push_back(i);
        }
    }

    void OptimizedParticleSystem::findNeighbors(uint16_t particle_index, std::vector<uint16_t>& neighbors)
    {
        neighbors.clear();
        
        float x = m_particles.positions_x[particle_index];
        float y = m_particles.positions_y[particle_index];
        float radius = m_particles.radii[particle_index];
        
        // Convert to grid coordinates
        int grid_x = static_cast<int>((x - m_spatial_grid.world_min.x) / m_spatial_grid.cell_size);
        int grid_y = static_cast<int>((y - m_spatial_grid.world_min.y) / m_spatial_grid.cell_size);
        
        // Check 3x3 grid around particle
        for (int dy = -1; dy <= 1; ++dy)
        {
            for (int dx = -1; dx <= 1; ++dx)
            {
                int check_x = grid_x + dx;
                int check_y = grid_y + dy;
                
                if (check_x < 0 || check_x >= m_spatial_grid.grid_width ||
                    check_y < 0 || check_y >= m_spatial_grid.grid_height)
                    continue;
                
                int cell_index = check_y * m_spatial_grid.grid_width + check_x;
                const auto& cell = m_spatial_grid.cells[cell_index];
                
                // Check all particles in this cell
                for (uint16_t other_index : cell)
                {
                    if (other_index == particle_index) continue;
                    
                    // Distance check
                    float dx_pos = x - m_particles.positions_x[other_index];
                    float dy_pos = y - m_particles.positions_y[other_index];
                    float distance_sq = dx_pos * dx_pos + dy_pos * dy_pos;
                    
                    float combined_radius = radius + m_particles.radii[other_index];
                    if (distance_sq < combined_radius * combined_radius)
                    {
                        neighbors.push_back(other_index);
                    }
                }
            }
        }
    }

    void OptimizedParticleSystem::resolveCollisions()
    {
        m_collision_checks = 0;
        m_contacts_found = 0;
        
        // Use contact caching to avoid redundant checks
        for (uint16_t i = 0; i < m_particles.count; ++i)
        {
            m_spatial_grid.temp_neighbors.clear();
            findNeighbors(i, m_spatial_grid.temp_neighbors);
            
            for (uint16_t j : m_spatial_grid.temp_neighbors)
            {
                if (j <= i) continue; // Avoid duplicate pairs
                
                m_collision_checks++;
                
                // Check if contact already exists
                uint32_t contact_key = (static_cast<uint32_t>(i) << 16) | j;
                if (contact_key < m_contact_cache.contact_exists.size() && 
                    m_contact_cache.contact_exists[contact_key])
                {
                    continue; // Skip cached contact
                }
                
                // Calculate collision response
                float dx = m_particles.positions_x[j] - m_particles.positions_x[i];
                float dy = m_particles.positions_y[j] - m_particles.positions_y[i];
                float distance = std::sqrt(dx * dx + dy * dy);
                
                float combined_radius = m_particles.radii[i] + m_particles.radii[j];
                if (distance < combined_radius && distance > 1e-6f)
                {
                    m_contacts_found++;
                    
                    // Normalize collision normal
                    float inv_distance = 1.0f / distance;
                    float nx = dx * inv_distance;
                    float ny = dy * inv_distance;
                    
                    // Calculate overlap
                    float overlap = combined_radius - distance;
                    
                    // Separate particles
                    float separation = overlap * 0.5f;
                    m_particles.positions_x[i] -= nx * separation;
                    m_particles.positions_y[i] -= ny * separation;
                    m_particles.positions_x[j] += nx * separation;
                    m_particles.positions_y[j] += ny * separation;
                    
                    // Velocity response (simplified)
                    float dvx = m_particles.velocities_x[j] - m_particles.velocities_x[i];
                    float dvy = m_particles.velocities_y[j] - m_particles.velocities_y[i];
                    float relative_velocity = dvx * nx + dvy * ny;
                    
                    if (relative_velocity < 0.0f) // Approaching
                    {
                        float restitution = 0.8f; // Bounce factor
                        float impulse = -(1.0f + restitution) * relative_velocity;
                        
                        m_particles.velocities_x[i] -= nx * impulse * 0.5f;
                        m_particles.velocities_y[i] -= ny * impulse * 0.5f;
                        m_particles.velocities_x[j] += nx * impulse * 0.5f;
                        m_particles.velocities_y[j] += ny * impulse * 0.5f;
                    }
                }
            }
        }
        
        // Update average neighbors for performance monitoring
        if (m_particles.count > 0)
        {
            m_average_neighbors = static_cast<float>(m_collision_checks) / m_particles.count;
        }
    }

    void OptimizedParticleSystem::integrateParticles(float dt)
    {
        // Symplectic Euler integration (more stable than forward Euler)
        for (uint16_t i = 0; i < m_particles.count; ++i)
        {
            // Update velocity first (with forces)
            m_particles.velocities_x[i] += 0.0f; // Add forces here
            m_particles.velocities_y[i] += -980.0f * dt; // Gravity
            
            // Apply damping
            float damping = 0.999f;
            m_particles.velocities_x[i] *= damping;
            m_particles.velocities_y[i] *= damping;
            
            // Update position
            m_particles.positions_x[i] += m_particles.velocities_x[i] * dt;
            m_particles.positions_y[i] += m_particles.velocities_y[i] * dt;
        }
    }

    void OptimizedParticleSystem::step(float dt)
    {
        // Update spatial grid
        updateSpatialGrid();
        
        // Integrate particles
        integrateParticles(dt);
        
        // Resolve collisions
        resolveCollisions();
        
        // Update islands (sleeping particles)
        updateIslands(dt);
    }

    void OptimizedParticleSystem::updateIslands(float dt)
    {
        // Simple island management - particles that haven't moved much go to sleep
        for (auto& island : m_islands)
        {
            bool all_sleeping = true;
            float max_velocity = 0.0f;
            
            for (uint16_t particle_index : island.particle_indices)
            {
                float vx = m_particles.velocities_x[particle_index];
                float vy = m_particles.velocities_y[particle_index];
                float velocity = std::sqrt(vx * vx + vy * vy);
                max_velocity = std::max(max_velocity, velocity);
                
                if (velocity > 1.0f) // Wake threshold
                {
                    all_sleeping = false;
                }
            }
            
            if (all_sleeping && max_velocity < 0.1f)
            {
                island.sleep_timer += dt;
                if (island.sleep_timer > 1.0f) // Sleep after 1 second of stillness
                {
                    island.is_sleeping = true;
                }
            }
            else
            {
                island.is_sleeping = false;
                island.sleep_timer = 0.0f;
            }
        }
    }
}
