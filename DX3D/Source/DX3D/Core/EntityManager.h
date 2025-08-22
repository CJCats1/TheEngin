#pragma once
#include <DX3D/Core/Entity.h>
#include <vector>
#include <unordered_map>
#include <memory>

namespace dx3d {
    class EntityManager {
    public:
        EntityManager() : m_nextEntityId(1) {}

        // Create a new entity
        Entity& createEntity(const std::string& name = "") {
            EntityId id = m_nextEntityId++;
            auto entity = std::make_unique<Entity>(id, name);
            Entity& ref = *entity;

            m_entities.push_back(std::move(entity));

            if (!name.empty()) {
                m_namedEntities[name] = &ref;
            }

            return ref;
        }

        // Find entity by name
        Entity* findEntity(const std::string& name) {
            auto it = m_namedEntities.find(name);
            return (it != m_namedEntities.end()) ? it->second : nullptr;
        }

        // Get all entities
        const std::vector<std::unique_ptr<Entity>>& getEntities() const {
            return m_entities;
        }

        // Get all entities with a specific component
        template<typename T>
        std::vector<Entity*> getEntitiesWithComponent() {
            std::vector<Entity*> result;
            for (auto& entity : m_entities) {
                if (entity->hasComponent<T>()) {
                    result.push_back(entity.get());
                }
            }
            return result;
        }

        // Clear all entities
        void clear() {
            m_entities.clear();
            m_namedEntities.clear();
            m_nextEntityId = 1;
        }

    private:
        std::vector<std::unique_ptr<Entity>> m_entities;
        std::unordered_map<std::string, Entity*> m_namedEntities;
        EntityId m_nextEntityId;
    };
}