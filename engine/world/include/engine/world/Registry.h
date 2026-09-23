#pragma once

#include "engine/world/SparseSet.h"
#include <unordered_map>
#include <typeindex>
#include <memory>

namespace Engine::World {

class Registry {
public:
    Registry() = default;
    ~Registry() = default;

    template <typename T>
    ComponentPool<T>& GetPool() {
        const std::type_index typeIdx(typeid(T));
        auto it = m_pools.find(typeIdx);
        if (it == m_pools.end()) {
            auto pool = std::make_unique<ComponentPool<T>>();
            auto* rawPtr = pool.get();
            m_pools[typeIdx] = std::move(pool);
            return *rawPtr;
        }
        return *static_cast<ComponentPool<T>*>(it->second.get());
    }

    template <typename T>
    [[nodiscard]] bool HasPool() const noexcept {
        const std::type_index typeIdx(typeid(T));
        return m_pools.find(typeIdx) != m_pools.end();
    }

    template <typename T>
    const ComponentPool<T>& GetPool() const {
        const std::type_index typeIdx(typeid(T));
        auto it = m_pools.find(typeIdx);
        ENGINE_ASSERT(it != m_pools.end(), "Component pool does not exist");
        return *static_cast<const ComponentPool<T>*>(it->second.get());
    }

    template <typename T, typename... Args>
    T& Emplace(EntityID entity, Args&&... args) {
        return GetPool<T>().Emplace(entity, std::forward<Args>(args)...);
    }

    template <typename T>
    [[nodiscard]] T& Get(EntityID entity) {
        return GetPool<T>().Get(entity);
    }

    template <typename T>
    [[nodiscard]] const T& Get(EntityID entity) const {
        return GetPool<T>().Get(entity);
    }

    template <typename T>
    [[nodiscard]] bool Has(EntityID entity) const noexcept {
        const std::type_index typeIdx(typeid(T));
        auto it = m_pools.find(typeIdx);
        return it != m_pools.end() && it->second->Has(entity);
    }

    template <typename T>
    void Remove(EntityID entity) {
        const std::type_index typeIdx(typeid(T));
        auto it = m_pools.find(typeIdx);
        if (it != m_pools.end()) {
            it->second->Remove(entity);
        }
    }

    void RemoveAll(EntityID entity) {
        for (auto& [_, pool] : m_pools) {
            pool->Remove(entity);
        }
    }

    void Clear() {
        for (auto& [_, pool] : m_pools) {
            pool->Clear();
        }
        m_pools.clear();
    }

private:
    std::unordered_map<std::type_index, std::unique_ptr<IComponentPool>> m_pools;
};

} // namespace Engine::World
