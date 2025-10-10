#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <typeindex>

namespace dx3d {
    using EntityId = std::size_t;

    class Entity {
    public:
        Entity(EntityId id, const std::string& name = "")
            : m_id(id), m_name(name) {
        }

        EntityId getId() const { return m_id; }
        const std::string& getName() const { return m_name; }
        void setName(const std::string& name) { m_name = name; }

        // Add a component to this entity
        template<typename T, typename... Args>
        T& addComponent(Args&&... args) {
            auto component = std::unique_ptr<T>(new T(std::forward<Args>(args)...));
            T& ref = *component;

            // Store the component with type erasure
            m_components[std::type_index(typeid(T))] = std::make_shared<ComponentWrapper<T>>(std::move(component));

            return ref;
        }

        // Get a component from this entity
        template<typename T>
        T* getComponent() {
            auto it = m_components.find(std::type_index(typeid(T)));
            if (it != m_components.end()) {
                auto wrapper = std::static_pointer_cast<ComponentWrapper<T>>(it->second);
                return wrapper->component.get();
            }
            return nullptr;
        }

        template<typename T>
        const T* getComponent() const {
            auto it = m_components.find(std::type_index(typeid(T)));
            if (it != m_components.end()) {
                auto wrapper = std::static_pointer_cast<ComponentWrapper<T>>(it->second);
                return wrapper->component.get();
            }
            return nullptr;
        }

        // Check if entity has a component
        template<typename T>
        bool hasComponent() const {
            return m_components.find(std::type_index(typeid(T))) != m_components.end();
        }

        // Remove a component
        template<typename T>
        void removeComponent() {
            m_components.erase(std::type_index(typeid(T)));
        }

    private:
        // Base class for component storage
        struct ComponentWrapperBase {
            virtual ~ComponentWrapperBase() = default;
        };

        // Templated wrapper for specific component types
        template<typename T>
        struct ComponentWrapper : ComponentWrapperBase {
            ComponentWrapper(std::unique_ptr<T> comp) : component(std::move(comp)) {}
            std::unique_ptr<T> component;
        };

        EntityId m_id;
        std::string m_name;
        std::unordered_map<std::type_index, std::shared_ptr<ComponentWrapperBase>> m_components;
    };
}