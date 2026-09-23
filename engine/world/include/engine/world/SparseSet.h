#pragma once

#include "engine/core/BaseTypes.h"
#include "engine/core/Assert.h"
#include "engine/world/Entity.h"
#include <vector>

namespace Engine::World {

class IComponentPool {
public:
    virtual ~IComponentPool() = default;
    virtual bool Has(EntityID entity) const noexcept = 0;
    virtual void Remove(EntityID entity) = 0;
    virtual void Clear() = 0;
};

template <typename T>
class ComponentPool final : public IComponentPool {
public:
    static constexpr Core::u32 INVALID_INDEX = 0xFFFFFFFF;

    ComponentPool() = default;
    ~ComponentPool() override = default;

    template <typename... Args>
    T& Emplace(EntityID entity, Args&&... args) {
        ENGINE_ASSERT(!Has(entity), "Component already exists on entity");

        if (entity >= m_sparse.size()) {
            m_sparse.resize(entity + 1, INVALID_INDEX);
        }

        const Core::u32 denseIndex = static_cast<Core::u32>(m_denseComponents.size());
        m_sparse[entity] = denseIndex;
        m_denseToEntity.push_back(entity);
        return m_denseComponents.emplace_back(std::forward<Args>(args)...);
    }

    [[nodiscard]] bool Has(EntityID entity) const noexcept override {
        return entity < m_sparse.size() && m_sparse[entity] != INVALID_INDEX;
    }

    [[nodiscard]] T& Get(EntityID entity) {
        ENGINE_ASSERT(Has(entity), "Entity does not possess component");
        return m_denseComponents[m_sparse[entity]];
    }

    [[nodiscard]] const T& Get(EntityID entity) const {
        ENGINE_ASSERT(Has(entity), "Entity does not possess component");
        return m_denseComponents[m_sparse[entity]];
    }

    void Remove(EntityID entity) override {
        if (!Has(entity)) return;

        const Core::u32 denseIndex = m_sparse[entity];
        const Core::u32 lastDenseIndex = static_cast<Core::u32>(m_denseComponents.size() - 1);
        const EntityID lastEntity = m_denseToEntity[lastDenseIndex];

        // Swap with last element for O(1) removal
        if (denseIndex != lastDenseIndex) {
            m_denseComponents[denseIndex] = std::move(m_denseComponents[lastDenseIndex]);
            m_denseToEntity[denseIndex] = lastEntity;
            m_sparse[lastEntity] = denseIndex;
        }

        m_denseComponents.pop_back();
        m_denseToEntity.pop_back();
        m_sparse[entity] = INVALID_INDEX;
    }

    void Clear() override {
        m_denseComponents.clear();
        m_denseToEntity.clear();
        m_sparse.clear();
    }

    [[nodiscard]] size_t Size() const noexcept { return m_denseComponents.size(); }
    [[nodiscard]] std::vector<T>& GetData() noexcept { return m_denseComponents; }
    [[nodiscard]] const std::vector<T>& GetData() const noexcept { return m_denseComponents; }
    [[nodiscard]] const std::vector<EntityID>& GetEntities() const noexcept { return m_denseToEntity; }

private:
    std::vector<T> m_denseComponents;
    std::vector<EntityID> m_denseToEntity;
    std::vector<Core::u32> m_sparse;
};

} // namespace Engine::World
