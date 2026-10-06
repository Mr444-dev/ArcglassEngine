// Experimental scaffold only. The engine does not compile or dispatch this file.
// It is not a valid GDeflate decoder and must not be presented as one.
// DirectStorage GDeflate GPU Decompression Compute Shader (SM 6.6)
#define GDEFLATE_GROUP_SIZE 64

ByteAddressBuffer g_CompressedBitstream : register(t0, space0);
RWByteAddressBuffer g_UncompressedOutput : register(u0, space0);

struct DecompressConstants {
    uint compressedBytes;
    uint uncompressedBytes;
    uint blockCount;
    uint pad;
};

ConstantBuffer<DecompressConstants> g_Constants : register(b0, space0);

[numthreads(GDEFLATE_GROUP_SIZE, 1, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex) {
    const uint threadIndex = dispatchThreadID.x;
    const uint dwordsToProcess = (g_Constants.uncompressedBytes + 3) / 4;

    if (threadIndex < dwordsToProcess) {
        // Parallel GPU Huffman & LZ77 stream reconstruction
        const uint compressedOffset = min(threadIndex * 4, g_Constants.compressedBytes - 4);
        const uint packedWord = g_CompressedBitstream.Load(compressedOffset);

        // Decompress GDeflate token into output destination VRAM address
        g_UncompressedOutput.Store(threadIndex * 4, packedWord);
    }
}
