#pragma once
#include "Entity.h"
#include <unordered_map>
#include <memory>
#include <set>
#include <bitset>

namespace dx3d {
    using Signature = std::bitset<32>;

    class System {
    public:
        std::set<EntityID> m_entities;
    };

    class SystemManager {
    public:
        template<typename T>
        std::shared_ptr<T> registerSystem() {
            const char* typeName = typeid(T).name();
            auto system = std::make_shared<T>();
            m_systems.insert({typeName, system});
            return system;
        }

        template<typename T>
        void setSignature(Signature signature) {
            const char* typeName = typeid(T).name();
            m_signatures.insert({typeName, signature});
        }

        void entityDestroyed(EntityID entity) {
            for (auto const& pair : m_systems) {
                auto const& system = pair.second;
                system->m_entities.erase(entity);
            }
        }

        void entitySignatureChanged(EntityID entity, Signature entitySignature) {
            for (auto const& pair : m_systems) {
                auto const& type = pair.first;
                auto const& system = pair.second;
                auto const& systemSignature = m_signatures[type];

                if ((entitySignature & systemSignature) == systemSignature) {
                    system->m_entities.insert(entity);
                } else {
                    system->m_entities.erase(entity);
                }
            }
        }

        template<typename T>
        std::shared_ptr<T> getSystem() {
            const char* typeName = typeid(T).name();
            return std::static_pointer_cast<T>(m_systems[typeName]);
        }

    private:
        std::unordered_map<const char*, Signature> m_signatures{};
        std::unordered_map<const char*, std::shared_ptr<System>> m_systems{};
    };
}