set(ENGINE_TEST_LIBRARIES
    Engine_Core
    Engine_Platform
    Engine_RHI
    Engine_Renderer
    Engine_World
    Engine_Assets
    Engine_Physics
    Engine_Animation
    Engine_Audio
    Engine_Editor_Lib
    Engine_Network
)

add_executable(Engine_Tests
    AllocatorTests.cpp
    JobSystemTests.cpp
    RenderGraphTests.cpp
    WorldTests.cpp
    AssetTests.cpp
    PhysicsTests.cpp
    EditorTests.cpp
    GPUDrivenTests.cpp
    AnimationAudioTests.cpp
    ProfilerTests.cpp
    DistributionTests.cpp
    NextGenRendererTests.cpp
    RayTracingTests.cpp
    MeshletGITests.cpp
    NetworkRollbackTests.cpp
    NeuralRadianceCacheTests.cpp
    WorldStreamingTests.cpp
    MeshShaderCullingTests.cpp
    AtmosphericSkyTests.cpp
    DirectStorageTests.cpp
)

target_link_libraries(Engine_Tests PRIVATE ${ENGINE_TEST_LIBRARIES})

# Keep ENGINE_ASSERT checks active in test translation units, including Release builds.
target_compile_definitions(Engine_Tests PRIVATE ENGINE_ENABLE_ASSERTS)

target_set_compiler_warnings(Engine_Tests)

add_test(NAME Engine_UnitTests COMMAND Engine_Tests)
set_tests_properties(Engine_UnitTests PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_executable(Engine_RHI_Smoke
    RHISmokeMain.cpp
    RHITests.cpp
)
target_link_libraries(Engine_RHI_Smoke PRIVATE ${ENGINE_TEST_LIBRARIES})
target_compile_definitions(Engine_RHI_Smoke PRIVATE ENGINE_ENABLE_ASSERTS)
target_set_compiler_warnings(Engine_RHI_Smoke)

add_test(NAME Engine_D3D12_Smoke COMMAND Engine_RHI_Smoke)
set_tests_properties(Engine_D3D12_Smoke PROPERTIES
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    LABELS "gpu;hardware"
)
