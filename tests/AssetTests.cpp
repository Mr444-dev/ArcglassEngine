#include "engine/core/BaseTypes.h"
#include "engine/core/Log.h"
#include "engine/core/Assert.h"
#include "engine/core/JobSystem.h"
#include "engine/assets/BinaryFormats.h"
#include "engine/assets/VirtualFileSystem.h"
#include "engine/assets/AssetDatabase.h"
#include "engine/assets/AssetCooker.h"
#include "engine/assets/AsyncAssetLoader.h"
#include <atomic>
#include <cstring>
#include <thread>

void TestAssetGUIDAndHashing() {
    using namespace Engine::Assets;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing AssetGUID & Content Hashing ---");

    AssetGUID g1 = AssetGUID::CreateRandom();
    std::string str = g1.ToString();
    AssetGUID g2 = AssetGUID::FromString(str);

    ENGINE_ASSERT(g1 == g2, "GUID string serialization/deserialization mismatch");

    const char* sampleData = "GameEngine2026_Standard_PBR_Geometry";
    AssetGUID contentGUID = AssetGUID::FromContent(sampleData, std::strlen(sampleData));
    ENGINE_ASSERT(contentGUID.IsValid(), "Content GUID invalid");

    ENGINE_LOG_INFO("AssetGUID verified: {} == {}", str, g2.ToString());
}

void TestVirtualFileSystem() {
    using namespace Engine::Assets;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing Virtual File System ---");

    auto& vfs = VirtualFileSystem::Get();
    vfs.Mount("test", "./build/vfs_test");

    const std::string testFile = "test://sample.bin";
    const std::vector<u8> writeData = {0xDE, 0xAD, 0xBE, 0xEF, 0x42, 0x13, 0x37};

    ENGINE_VERIFY(vfs.WriteFile(testFile, writeData.data(), writeData.size()), "VFS write failed");
    ENGINE_ASSERT(vfs.FileExists(testFile), "VFS FileExists returned false for created file");

    std::vector<u8> readData;
    ENGINE_VERIFY(vfs.ReadFile(testFile, readData), "VFS read failed");
    ENGINE_ASSERT(readData == writeData, "VFS read data does not match written data");
    ENGINE_ASSERT(vfs.ResolvePhysicalPath("test://../escape.bin").empty(), "VFS accepted parent traversal outside a mount");
    ENGINE_ASSERT(vfs.ResolvePhysicalPath("missing://file.bin").empty(), "VFS resolved an unknown mount");
    ENGINE_ASSERT(!vfs.WriteFile("test://../escape.bin", writeData.data(), writeData.size()), "VFS wrote outside a mount");

    ENGINE_LOG_INFO("VirtualFileSystem verified: written and read back {} bytes.", readData.size());
}

void TestMeshCookingAndMeshlets() {
    using namespace Engine::Assets;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing Mesh Cooking & Meshlet Generation ---");

    // Construct a procedural cube (24 vertices, 36 indices)
    std::vector<MeshVertex> vertices;
    std::vector<u32> indices;

    const Vec3 p[8] = {
        {-1, -1, -1}, { 1, -1, -1}, { 1,  1, -1}, {-1,  1, -1},
        {-1, -1,  1}, { 1, -1,  1}, { 1,  1,  1}, {-1,  1,  1}
    };

    const int faceIndices[6][4] = {
        {0, 1, 2, 3}, {5, 4, 7, 6}, {4, 0, 3, 7},
        {1, 5, 6, 2}, {4, 5, 1, 0}, {3, 2, 6, 7}
    };

    for (int f = 0; f < 6; ++f) {
        u32 base = static_cast<u32>(vertices.size());
        for (int v = 0; v < 4; ++v) {
            MeshVertex vert{};
            vert.position = p[faceIndices[f][v]];
            vert.normal = {0.0f, 1.0f, 0.0f};
            vertices.push_back(vert);
        }
        indices.push_back(base + 0); indices.push_back(base + 1); indices.push_back(base + 2);
        indices.push_back(base + 0); indices.push_back(base + 2); indices.push_back(base + 3);
    }

    AssetGUID cubeGUID = AssetGUID::CreateRandom();
    const std::string meshPath = "test://cube.cmesh";
    ENGINE_VERIFY(AssetCooker::CookMesh(vertices, indices, meshPath, cubeGUID), "Failed cooking mesh");

    std::vector<u32> invalidIndices = indices;
    invalidIndices.front() = static_cast<u32>(vertices.size());
    ENGINE_ASSERT(!AssetCooker::CookMesh(vertices, invalidIndices, "test://invalid.cmesh"), "Mesh cooker accepted an out-of-range index");

    ENGINE_LOG_INFO("Mesh cooking verified for procedural cube.");
}

void TestTextureCookingAndMips() {
    using namespace Engine::Assets;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing Texture Cooking & MIP Generation ---");

    constexpr u32 WIDTH = 64;
    constexpr u32 HEIGHT = 64;
    std::vector<u8> pixels(WIDTH * HEIGHT * 4, 180); // Solid gray texture

    const std::string texPath = "test://checker.ctex";
    ENGINE_VERIFY(AssetCooker::CookTexture(WIDTH, HEIGHT, pixels.data(), pixels.size(), texPath), "Failed cooking texture");

    std::vector<u8> rawBytes;
    ENGINE_VERIFY(VirtualFileSystem::Get().ReadFile(texPath, rawBytes), "Failed reading cooked texture");

    ENGINE_ASSERT(rawBytes.size() >= sizeof(TextureFileHeader), "Cooked texture header is truncated");
    TextureFileHeader header{};
    std::memcpy(&header, rawBytes.data(), sizeof(header));
    ENGINE_ASSERT(header.commonHeader.magic == CTEX_MAGIC, "Texture magic mismatch");
    ENGINE_ASSERT(header.mipCount == 7, "Expected 7 MIP levels for 64x64 texture (64..1)");

    ENGINE_LOG_INFO("Texture cooking verified ({}x{}, {} MIPs generated).", WIDTH, HEIGHT, header.mipCount);
}

void TestAsyncAssetLoadingAndBenchmark() {
    using namespace Engine::Assets;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing Mesh Loading ---");

    const std::string meshPath = "test://cube.cmesh";

    // 1. Synchronous loading benchmark
    auto loadedMesh = AsyncAssetLoader::LoadMesh(meshPath);
    ENGINE_ASSERT(loadedMesh->isValid, "Synchronous mesh load failed");
    ENGINE_ASSERT(loadedMesh->vertices.size() == 24, "Vertex count mismatch in loaded mesh");
    ENGINE_ASSERT(loadedMesh->indices.size() == 36, "Index count mismatch in loaded mesh");
    for (const auto& meshlet : loadedMesh->meshlets) {
        ENGINE_ASSERT(meshlet.vertexCount <= 64 && meshlet.triangleCount <= 126, "Cooked meshlet exceeded its declared limits");
    }

    ENGINE_LOG_INFO("Loaded .cmesh synchronously ({} vertices, {} indices).", loadedMesh->vertices.size(), loadedMesh->indices.size());

    // 2. Asynchronous job loading
    JobSystem::Get().Initialize();

    std::atomic<bool> asyncDone{false};
    AsyncAssetLoader::LoadMeshAsync(meshPath, [&](std::shared_ptr<LoadedMesh> asyncMesh) {
        ENGINE_ASSERT(asyncMesh->isValid, "Async mesh load failed");
        asyncDone.store(true, std::memory_order_release);
    });

    // Wait for async task to complete via job system
    while (!asyncDone.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }

    JobSystem::Get().Shutdown();
    ENGINE_LOG_INFO("Asynchronous JobSystem mesh loading completed successfully.");

    bool synchronousFallbackCompleted = false;
    AsyncAssetLoader::LoadMeshAsync(meshPath, [&](std::shared_ptr<LoadedMesh> fallbackMesh) {
        synchronousFallbackCompleted = fallbackMesh && fallbackMesh->isValid;
    });
    ENGINE_ASSERT(synchronousFallbackCompleted, "Async asset request did not complete when the JobSystem was stopped");

    // 3. Defensive Corrupted File Guardrail
    const std::string corruptedPath = "test://corrupted.cmesh";
    const std::vector<u8> badBytes = {0x00, 0x00, 0x00, 0x00, 0x11, 0x22}; // Invalid header
    VirtualFileSystem::Get().WriteFile(corruptedPath, badBytes.data(), badBytes.size());

    auto badMesh = AsyncAssetLoader::LoadMesh(corruptedPath);
    ENGINE_ASSERT(!badMesh->isValid, "Corrupted mesh should not be valid!");
    ENGINE_LOG_INFO("Corrupted asset guardrail successfully caught invalid header magic.");

    const std::string truncatedPath = "test://truncated.cmesh";
    std::vector<u8> truncatedBytes;
    ENGINE_VERIFY(VirtualFileSystem::Get().ReadFile(meshPath, truncatedBytes), "Failed reading valid mesh for truncation test");
    truncatedBytes.resize(truncatedBytes.size() - 1);
    ENGINE_VERIFY(VirtualFileSystem::Get().WriteFile(truncatedPath, truncatedBytes.data(), truncatedBytes.size()), "Failed writing truncated mesh fixture");
    ENGINE_ASSERT(!AsyncAssetLoader::LoadMesh(truncatedPath)->isValid, "Truncated mesh must be rejected");
}

void RunAssetUnitTests() {
    TestAssetGUIDAndHashing();
    TestVirtualFileSystem();
    TestMeshCookingAndMeshlets();
    TestTextureCookingAndMips();
    TestAsyncAssetLoadingAndBenchmark();
}
