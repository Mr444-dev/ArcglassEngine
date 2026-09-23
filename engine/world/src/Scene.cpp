#include "engine/world/Scene.h"
#include "engine/core/Log.h"
#include <algorithm>

namespace Engine::World {

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

    // Add baseline components
    m_registry.Emplace<TagComponent>(id, name);
    m_registry.Emplace<TransformComponent>(id);

    m_hierarchy.MarkDirty();
    return Entity(id, this);
}

void Scene::DestroyEntity(EntityID entity) {
    auto it = std::find(m_entities.begin(), m_entities.end(), entity);
    if (it != m_entities.end()) {
        m_hierarchy.DetachChild(entity, m_registry);
        m_registry.RemoveAll(entity);
        m_entities.erase(it);
        m_hierarchy.MarkDirty();
    }
}

void Scene::Update(float deltaTime) {
    (void)deltaTime;

    // 1. Update entire hierarchical transform chain in linear DFS order
    m_hierarchy.UpdateWorldTransforms(m_registry);

    // 2. Update mesh world AABBs from transforms
    auto& meshPool = m_registry.GetPool<MeshComponent>();
    auto& meshes = meshPool.GetData();
    const auto& meshEntities = meshPool.GetEntities();

    std::vector<std::pair<EntityID, Core::AABB>> bvhObjects;
    bvhObjects.reserve(meshes.size());

    for (size_t i = 0; i < meshes.size(); ++i) {
        EntityID e = meshEntities[i];
        auto& mesh = meshes[i];

        if (m_registry.Has<TransformComponent>(e)) {
            const auto& t = m_registry.Get<TransformComponent>(e);
            mesh.worldAABB.min = mesh.localAABB.min + t.position;
            mesh.worldAABB.max = mesh.localAABB.max + t.position;
        }

        if (mesh.isVisible) {
            bvhObjects.emplace_back(e, mesh.worldAABB);
        }
    }

    // 3. Rebuild or update Spatial BVH
    m_bvh.Build(bvhObjects);
}

} // namespace Engine::World
