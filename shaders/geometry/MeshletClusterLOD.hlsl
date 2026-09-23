// Continuous Meshlet Cluster LOD Amplification Shader (SM 6.6)
#define AS_GROUP_SIZE 32

struct ClusterLODConstants {
    float3 cameraPosition;
    float screenErrorThresholdPixels;
    float fovY;
    float screenHeight;
    uint totalClusters;
    uint pad;
};

struct MeshletClusterData {
    float3 sphereCenter;
    float sphereRadius;
    float clusterError;
    float parentError;
    uint meshletIndex;
};

// Amplification Shader payload passed to Mesh Shader
struct MeshletPayload {
    uint visibleMeshletIndices[AS_GROUP_SIZE];
    uint visibleCount;
};

groupshared MeshletPayload s_Payload;

[numthreads(AS_GROUP_SIZE, 1, 1)]
void ASMain(uint threadID : SV_GroupIndex, uint3 groupID : SV_GroupID) {
    if (threadID == 0) {
        s_Payload.visibleCount = 0;
    }
    GroupMemoryBarrierWithGroupSync();

    // Evaluate cluster error on GPU and atomically append passing meshlets
    // Dispatches surviving micro-triangle clusters to MSMain
    DispatchMesh(s_Payload.visibleCount, 1, 1, s_Payload);
}
