#include "engine/world/Scene.h"
#include "engine/core/Log.h"
#include <algorithm>
#include <unordered_set>
#include <vector>

namespace Engine::World {
namespace {

Core::Vec3 TransformPoint(const Core::Mat4x4& matrix, const Core::Vec3& point) noexcept {
    return {
        point.x * matrix.m[0] + point.y * matrix.m[4] + point.z * matrix.m[8] + matrix.m[12],
        point.x * matrix.m[1] + point.y * matrix.m[5] + point.z * matrix.m[9] + matrix.m[13],
        point.x * matrix.m[2] + point.y * matrix.m[6] + point.z * matrix.m[10] + matrix.m[14]
    };
}

Core::AABB TransformBounds(const Core::AABB& localBounds, const Core::Mat4x4& worldMatrix) noexcept {
    const Core::Vec3 firstCorner = TransformPoint(worldMatrix, localBounds.min);
    Core::AABB worldBounds{firstCorner, firstCorner};
    for (Core::u32 corner = 1; corner < 8; ++corner) {
        const Core::Vec3 point{
            (corner & 1) ? localBounds.max.x : localBounds.min.x,
            (corner & 2) ? localBounds.max.y : localBounds.min.y,
            (corner & 4) ? localBounds.max.z : localBounds.min.z
        };
        const Core::Vec3 transformed = TransformPoint(worldMatrix, point);
        worldBounds.Encapsulate(Core::AABB{transformed, transformed});
    }
    return worldBounds;
}

} // namespace

Scene::Scene() {
    ENGINE_LOG_INFO("World Scene initialized.");
}

Scene::~Scene() {
    m_bvh.Clear();
    m_entities.clear();
    m_registry.Clear();
}

Entity Scene::CreateEntity(const std::string& name) {
    const EntityID id = m_nextEntityID++;
    m_entities.push_back(id);
    m_registry.Emplace<TagComponent>(id, name);
    m_registry.Emplace<TransformComponent>(id);
    m_hierarchy.MarkDirty();
    return Entity(id, this);
}

void Scene::DestroyEntity(EntityID entity) {
    if (std::find(m_entities.begin(), m_entities.end(), entity) == m_entities.end()) return;

    // Destroying a scene node destroys its complete child subtree, matching the editor's
    // hierarchy semantics and preventing stale child-to-parent references.
    std::vector<EntityID> pending{entity};
    std::vector<EntityID> subtree;
    std::unordered_set<EntityID> visited;
    visited.reserve(m_entities.size());

    while (!pending.empty()) {
        const EntityID current = pending.back();
        pending.pop_back();
        if (!visited.insert(current).second) continue;
        subtree.push_back(current);
        if (!m_registry.Has<TransformComponent>(current)) continue;

        std::unordered_set<EntityID> siblingChain;
        EntityID child = m_registry.Get<TransformComponent>(current).firstChild;
        while (child != NULL_ENTITY && siblingChain.insert(child).second && siblingChain.size() <= m_entities.size()) {
            if (!m_registry.Has<TransformComponent>(child)) break;
            const EntityID nextSibling = m_registry.Get<TransformComponent>(child).nextSibling;
            pending.push_back(child);
            child = nextSibling;
        }
    }

    m_hierarchy.DetachChild(entity, m_registry);
    for (const EntityID removed : subtree) {
        if (m_registry.Has<TransformComponent>(removed)) {
            auto& transform = m_registry.Get<TransformComponent>(removed);
            transform.parent = NULL_ENTITY;
            transform.firstChild = NULL_ENTITY;
            transform.nextSibling = NULL_ENTITY;
            transform.prevSibling = NULL_ENTITY;
        }
        m_registry.RemoveAll(removed);
    }

    m_entities.erase(std::remove_if(m_entities.begin(), m_entities.end(), [&](EntityID candidate) {
        return visited.contains(candidate);
    }), m_entities.end());
    m_bvh.Clear();
    m_hierarchy.MarkDirty();
}

void Scene::Update(float deltaTime) {
    (void)deltaTime;
    m_hierarchy.UpdateWorldTransforms(m_registry);

    auto& meshPool = m_registry.GetPool<MeshComponent>();
    auto& meshes = meshPool.GetData();
    const auto& meshEntities = meshPool.GetEntities();
    std::vector<std::pair<EntityID, Core::AABB>> bvhObjects;
    bvhObjects.reserve(meshes.size());

    for (size_t i = 0; i < meshes.size(); ++i) {
        const EntityID entity = meshEntities[i];
        auto& mesh = meshes[i];
        if (!m_registry.Has<TransformComponent>(entity)) continue;

        const auto& transform = m_registry.Get<TransformComponent>(entity);
        mesh.worldAABB = TransformBounds(mesh.localAABB, transform.worldMatrix);
        if (mesh.isVisible) bvhObjects.emplace_back(entity, mesh.worldAABB);
    }

    m_bvh.Build(bvhObjects);
}

} // namespace Engine::World
