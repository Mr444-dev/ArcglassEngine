#include "engine/core/BaseTypes.h"
#include "engine/core/Log.h"
#include "engine/core/Assert.h"
#include "engine/animation/Skeleton.h"
#include "engine/animation/AnimationClip.h"
#include "engine/animation/Animator.h"
#include "engine/animation/SkinningPass.h"
#include "engine/audio/AudioTypes.h"
#include "engine/audio/SpatialAudio.h"
#include "engine/audio/AudioEngine.h"
#include "engine/world/Scene.h"
#include "engine/world/Components.h"

void TestSkeletonAndJointHierarchy() {
    using namespace Engine::Animation;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing Skeleton Joint Hierarchy ---");

    Skeleton skeleton;

    // Build hierarchy: Root (0) -> Spine (1) -> Hand (2)
    Mat4x4 invRoot = Mat4x4::Translation({0.0f, 0.0f, 0.0f});
    Mat4x4 invSpine = Mat4x4::Translation({0.0f, -1.0f, 0.0f});
    Mat4x4 invHand = Mat4x4::Translation({0.0f, -2.0f, 0.0f});

    i16 rootIdx = skeleton.AddJoint("Root", -1, invRoot);
    i16 spineIdx = skeleton.AddJoint("Spine", rootIdx, invSpine);
    i16 handIdx = skeleton.AddJoint("Hand", spineIdx, invHand);

    ENGINE_ASSERT(rootIdx == 0, "Root joint index mismatch");
    ENGINE_ASSERT(spineIdx == 1, "Spine joint index mismatch");
    ENGINE_ASSERT(handIdx == 2, "Hand joint index mismatch");
    ENGINE_ASSERT(skeleton.FindJointIndex("Spine") == 1, "FindJointIndex failed");

    ENGINE_LOG_INFO("Skeleton joint hierarchy passed ({} joints validated).", skeleton.GetJointCount());
}

void TestSkeletalAnimationAndSkinningDeformation() {
    using namespace Engine::Animation;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Milestone Phase 8: Skeletal Animation & Compute Skinning ---");

    Skeleton skeleton;
    Mat4x4 invRoot = Mat4x4::Translation({0.0f, 0.0f, 0.0f});
    Mat4x4 invHand = Mat4x4::Translation({0.0f, -2.0f, 0.0f});
    i16 root = skeleton.AddJoint("Root", -1, invRoot);
    i16 hand = skeleton.AddJoint("Hand", root, invHand);

    AnimationClip clip("Reach", 1.0f);

    JointTrack handTrack;
    handTrack.jointIndex = hand;

    // Keyframe 0: at (0, 2, 0)
    TransformKey k0{};
    k0.time = 0.0f;
    k0.translation = {0.0f, 2.0f, 0.0f};
    handTrack.keys.push_back(k0);

    // Keyframe 1: translated to (5, 2, 0)
    TransformKey k1{};
    k1.time = 1.0f;
    k1.translation = {5.0f, 2.0f, 0.0f};
    handTrack.keys.push_back(k1);

    clip.AddTrack(std::move(handTrack));

    Animator animator(&skeleton);
    animator.PlayClip(&clip);

    // Evaluate at t = 0.5s (halfway: translation X = 2.5f)
    animator.Update(0.5f);

    const auto& palette = animator.GetSkinningPalette();
    ENGINE_ASSERT(palette.size() == 2, "Palette size mismatch");

    // Test vertex bound 100% to Hand joint (index 1)
    std::vector<SkinnedVertex> verts(1);
    verts[0].position = {0.0f, 2.0f, 0.0f}; // At bind pose hand position
    verts[0].normal = {0.0f, 1.0f, 0.0f};
    verts[0].boneIndices[0] = static_cast<u32>(hand);
    verts[0].boneWeights[0] = 1.0f;

    std::vector<Vec3> deformedPos;
    std::vector<Vec3> deformedNormals;
    SkinningPass::DeformVertices(verts, palette, deformedPos, deformedNormals);

    ENGINE_ASSERT(deformedPos.size() == 1, "Deformed vertex size mismatch");
    // Hand moved by 2.5 on X
    ENGINE_ASSERT(std::abs(deformedPos[0].x - 2.5f) < 1e-3f, "Vertex deformation X mismatch");
    ENGINE_LOG_INFO("Skinned vertex deformed from (0, 2, 0) to ({:.2f}, {:.2f}, {:.2f}).",
        deformedPos[0].x, deformedPos[0].y, deformedPos[0].z);

    ENGINE_LOG_INFO("Skeletal animation and Linear Blend Skinning passed.");
}

void TestSpatialAudioAttenuationAndPanning() {
    using namespace Engine::Audio;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing Spatial 3D Audio & Attenuation ---");

    // 1. Attenuation checks
    float attenNear = SpatialAudio::CalculateAttenuation(1.0f, 2.0f, 50.0f);
    ENGINE_ASSERT(attenNear == 1.0f, "Sound within minDistance should be full volume");

    float attenFar = SpatialAudio::CalculateAttenuation(60.0f, 2.0f, 50.0f);
    ENGINE_ASSERT(attenFar == 0.0f, "Sound beyond maxDistance should be silent");

    float attenMid = SpatialAudio::CalculateAttenuation(20.0f, 2.0f, 50.0f);
    ENGINE_ASSERT(attenMid > 0.0f && attenMid < 1.0f, "Mid-range attenuation invalid");

    // 2. Stereo Panning checks
    Vec3 listenerPos{0.0f, 0.0f, 0.0f};
    Vec3 listenerForward{0.0f, 0.0f, 1.0f};
    Vec3 listenerUp{0.0f, 1.0f, 0.0f};

    // Right sound
    float leftGain = 0.0f, rightGain = 0.0f;
    SpatialAudio::CalculateStereoPan({10.0f, 0.0f, 0.0f}, listenerPos, listenerForward, listenerUp, leftGain, rightGain);
    ENGINE_ASSERT(rightGain > leftGain, "Sound to the right should pan to right channel");

    // Left sound
    SpatialAudio::CalculateStereoPan({-10.0f, 0.0f, 0.0f}, listenerPos, listenerForward, listenerUp, leftGain, rightGain);
    ENGINE_ASSERT(leftGain > rightGain, "Sound to the left should pan to left channel");

    // Center sound
    SpatialAudio::CalculateStereoPan({0.0f, 0.0f, 10.0f}, listenerPos, listenerForward, listenerUp, leftGain, rightGain);
    ENGINE_ASSERT(std::abs(leftGain - rightGain) < 1e-3f, "Center sound should be balanced in stereo");

    ENGINE_LOG_INFO("Spatial 3D Audio attenuation and stereo panning passed.");
}

void TestAudioEngineIntegration() {
    using namespace Engine::Audio;
    using namespace Engine::World;
    using namespace Engine::Core;

    Logger::LogMessage(LogLevel::Info, "--- Testing AudioEngine Scene Integration ---");

    AudioEngine& audio = AudioEngine::Get();
    audio.Initialize();

    Scene scene;
    Entity listener = scene.CreateEntity("PlayerListener");
    listener.AddComponent<AudioListenerComponent>();

    Entity speaker = scene.CreateEntity("SirenSpeaker");
    auto& src = speaker.AddComponent<AudioSourceComponent>();
    src.isPlaying = true;
    src.minDistance = 2.0f;
    src.maxDistance = 100.0f;
    src.spatialBlend = 1.0f;

    auto& t = speaker.GetComponent<TransformComponent>();
    t.position = {15.0f, 0.0f, 0.0f}; // 15 units to the right

    audio.Update(0.016f, scene);

    ENGINE_ASSERT(audio.GetActiveSourceCount() == 1, "Expected 1 active audio source");
    ENGINE_ASSERT(src.currentRightGain > src.currentLeftGain, "Audio source should reflect spatial panning in scene");

    audio.Shutdown();
    ENGINE_LOG_INFO("AudioEngine scene integration verified.");
}

void RunAnimationAudioUnitTests() {
    TestSkeletonAndJointHierarchy();
    TestSkeletalAnimationAndSkinningDeformation();
    TestSpatialAudioAttenuationAndPanning();
    TestAudioEngineIntegration();
}
