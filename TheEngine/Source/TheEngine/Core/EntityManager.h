#pragma once
#include <TheEngine/Core/Entity.h>
#include <TheEngine/Core/Export.h>
#include <vector>
#include <unordered_map>
#include <memory>
#include <string>
#include <algorithm>

namespace TheEngine {
    class THEENGINE_API EntityManager {
    public:
        EntityManager() : m_nextEntityId(1) {}
        EntityManager(const EntityManager&) = delete;
        EntityManager& operator=(const EntityManager&) = delete;

        // Create a new entity
        Entity& createEntity(const std::string& name = "") {
            EntityId id = m_nextEntityId++;
            
            // Create entity with raw pointer to avoid unique_ptr issues
            Entity* entity = new Entity(id, name);
            m_entities.push_back(std::unique_ptr<Entity>(entity));
            Entity& ref = *entity;

            if (!name.empty()) {
                m_namedEntities[name] = &ref;
            }

            return ref;
        }
        bool removeEntity(const std::string& name) {
            auto it = m_namedEntities.find(name);
            if (it != m_namedEntities.end()) {
                Entity* entityToRemove = it->second;

                // Remove from named entities map
                m_namedEntities.erase(it);

                // Remove from entities vector (swap-with-back-and-pop to avoid unique_ptr copy)
                for (size_t i = 0; i < m_entities.size(); ++i) {
                    if (m_entities[i].get() == entityToRemove) {
                        if (i != m_entities.size() - 1) {
                            m_entities[i] = std::move(m_entities.back());
                        }
                        m_entities.pop_back();
                        break;
                    }
                }

                return true;
            }
            return false;
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