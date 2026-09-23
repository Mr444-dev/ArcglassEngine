// Modern 3D Game Engine - Fullscreen Interactive 3D Crystal Seeker Game
// HLSL SM 5.1 / 6.0 Compatible

cbuffer PushConstants : register(b0) {
    float Time;
    float ScreenWidth;
    float ScreenHeight;
    float CamYaw;
    float CamPitch;
    float CamX;
    float CamY;
    float CamZ;
    float PlayerX;
    float PlayerY;
    float PlayerZ;
    float PlayerYaw;
    float CrystalMask; // bitmask of collected crystals (0..31)
    float Score;
    float Speed;
    float GameWon;
};

struct VSOutput {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

VSOutput VSMain(uint vertexID : SV_VertexID) {
    VSOutput output;
    float2 uvs[3] = {
        float2(0.0f, 0.0f),
        float2(2.0f, 0.0f),
        float2(0.0f, 2.0f)
    };
    output.uv = uvs[vertexID % 3];
    output.position = float4(output.uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    return output;
}

// 4x6 Pixel Embedded Bitmap Font Glyphs
uint GetGlyph(uint c) {
    if (c == 'A') return 0x999F96;
    if (c == 'B') return 0xE99E9E;
    if (c == 'C') return 0x788887;
    if (c == 'D') return 0xE9999E;
    if (c == 'E') return 0xF88E8F;
    if (c == 'F') return 0x888E8F;
    if (c == 'G') return 0x799B87;
    if (c == 'H') return 0x999F99;
    if (c == 'I') return 0xE4444E;
    if (c == 'J') return 0x699111;
    if (c == 'K') return 0x9ACCAC;
    if (c == 'L') return 0xF88888;
    if (c == 'M') return 0x9999F9;
    if (c == 'N') return 0x999BD9;
    if (c == 'O') return 0x699996;
    if (c == 'P') return 0x888E9E;
    if (c == 'R') return 0x99AE9E;
    if (c == 'S') return 0x691687;
    if (c == 'T') return 0x66666F;
    if (c == 'U') return 0x699999;
    if (c == 'V') return 0x669999;
    if (c == 'W') return 0x9F9999;
    if (c == 'X') return 0x966669;
    if (c == 'Y') return 0x666699;
    if (c == 'Z') return 0xF8421F;
    if (c == '0') return 0x699996;
    if (c == '1') return 0xE44464;
    if (c == '2') return 0xF8421F;
    if (c == '3') return 0xE1161F;
    if (c == '4') return 0x11F999;
    if (c == '5') return 0xF11E8F;
    if (c == '6') return 0x699E86;
    if (c == '7') return 0x22211F;
    if (c == '8') return 0x699696;
    if (c == '9') return 0x611F96;
    if (c == ':') return 0x060600;
    if (c == '/') return 0x124880;
    if (c == '!') return 0x404444;
    return 0x000000;
}

bool IsChar(float2 px, float2 pos, uint c, float scale) {
    float2 rel = (px - pos) / scale;
    if (rel.x < 0.0f || rel.x >= 4.0f || rel.y < 0.0f || rel.y >= 6.0f) return false;
    uint x = 3 - uint(rel.x); // Fixed: read MSB to LSB
    uint y = uint(rel.y);
    uint glyph = GetGlyph(c);
    return ((glyph >> (y * 4 + x)) & 1) != 0;
}

bool Str4(float2 px, float2 pos, uint c0, uint c1, uint c2, uint c3, float scale = 2.0f) {
    float w = 5.0f * scale;
    if (IsChar(px, pos + float2(0.0f * w, 0.0f), c0, scale)) return true;
    if (IsChar(px, pos + float2(1.0f * w, 0.0f), c1, scale)) return true;
    if (IsChar(px, pos + float2(2.0f * w, 0.0f), c2, scale)) return true;
    if (IsChar(px, pos + float2(3.0f * w, 0.0f), c3, scale)) return true;
    return false;
}

bool Str5(float2 px, float2 pos, uint c0, uint c1, uint c2, uint c3, uint c4, float scale = 2.0f) {
    float w = 5.0f * scale;
    if (Str4(px, pos, c0, c1, c2, c3, scale)) return true;
    if (IsChar(px, pos + float2(4.0f * w, 0.0f), c4, scale)) return true;
    return false;
}

bool Str6(float2 px, float2 pos, uint c0, uint c1, uint c2, uint c3, uint c4, uint c5, float scale = 2.0f) {
    float w = 5.0f * scale;
    if (Str4(px, pos, c0, c1, c2, c3, scale)) return true;
    if (IsChar(px, pos + float2(4.0f * w, 0.0f), c4, scale)) return true;
    if (IsChar(px, pos + float2(5.0f * w, 0.0f), c5, scale)) return true;
    return false;
}

bool Str7(float2 px, float2 pos, uint c0, uint c1, uint c2, uint c3, uint c4, uint c5, uint c6, float scale = 2.0f) {
    float w = 5.0f * scale;
    if (Str6(px, pos, c0, c1, c2, c3, c4, c5, scale)) return true;
    if (IsChar(px, pos + float2(6.0f * w, 0.0f), c6, scale)) return true;
    return false;
}

bool Str8(float2 px, float2 pos, uint c0, uint c1, uint c2, uint c3, uint c4, uint c5, uint c6, uint c7, float scale = 2.0f) {
    float w = 5.0f * scale;
    if (Str7(px, pos, c0, c1, c2, c3, c4, c5, c6, scale)) return true;
    if (IsChar(px, pos + float2(7.0f * w, 0.0f), c7, scale)) return true;
    return false;
}

// Ray-box intersection helper
bool RayBoxIntersect(float3 ro, float3 rd, float3 boxMin, float3 boxMax, out float tNear, out float3 normal) {
    float3 invD = 1.0f / rd;
    float3 t0 = (boxMin - ro) * invD;
    float3 t1 = (boxMax - ro) * invD;
    float3 tMin = min(t0, t1);
    float3 tMax = max(t0, t1);

    float tEnter = max(max(tMin.x, tMin.y), tMin.z);
    float tExit  = min(min(tMax.x, tMax.y), tMax.z);

    if (tEnter <= tExit && tExit > 0.0f) {
        tNear = tEnter > 0.0f ? tEnter : tExit;
        float3 hitPoint = ro + rd * tNear;
        float3 center = (boxMin + boxMax) * 0.5f;
        float3 size = (boxMax - boxMin) * 0.5f;
        float3 d = (hitPoint - center) / size;
        float3 absD = abs(d);
        if (absD.x > absD.y && absD.x > absD.z) normal = float3(sign(d.x), 0.0f, 0.0f);
        else if (absD.y > absD.z) normal = float3(0.0f, sign(d.y), 0.0f);
        else normal = float3(0.0f, 0.0f, sign(d.z));
        return true;
    }
    tNear = 1e9;
    normal = float3(0.0f, 1.0f, 0.0f);
    return false;
}

// Ray-Sphere intersection helper
bool RaySphereIntersect(float3 ro, float3 rd, float3 center, float radius, out float tNear) {
    float3 oc = ro - center;
    float b = dot(oc, rd);
    float c = dot(oc, oc) - radius * radius;
    float h = b * b - c;
    if (h < 0.0f) {
        tNear = 1e9;
        return false;
    }
    h = sqrt(h);
    tNear = -b - h;
    if (tNear < 0.0f) tNear = -b + h;
    return tNear > 0.0f;
}

float4 PSMain(VSOutput input) : SV_Target {
    float2 px = input.position.xy;
    float w = ScreenWidth;
    float h = ScreenHeight;
    float aspect = w / h;

    float2 uv = (px / float2(w, h)) * 2.0f - 1.0f;
    uv.y = -uv.y;
    uv.x *= aspect;

    // 1. Third-Person Follow Camera behind Player
    float3 playerPos = float3(PlayerX, PlayerY, PlayerZ);
    float3 forward = normalize(float3(
        sin(CamYaw) * cos(CamPitch),
        sin(CamPitch),
        cos(CamYaw) * cos(CamPitch)
    ));
    float3 right = normalize(cross(forward, float3(0.0f, 1.0f, 0.0f)));
    float3 up = normalize(cross(right, forward));

    float3 ro = float3(CamX, CamY, CamZ);
    float3 rd = normalize(uv.x * right + uv.y * up + 1.6f * forward);

    // 2. Atmospheric Sky Gradient & Sun
    float horizonFactor = clamp(rd.y * 2.5f + 0.2f, 0.0f, 1.0f);
    float3 skyZenith = float3(0.04f, 0.06f, 0.12f);
    float3 skyHorizon = float3(0.18f, 0.22f, 0.35f);
    float3 sceneCol = lerp(skyHorizon, skyZenith, horizonFactor);

    float3 sunDir = normalize(float3(0.5f, 0.6f, 0.4f));
    float sunDot = max(dot(rd, sunDir), 0.0f);
    sceneCol += float3(1.0f, 0.9f, 0.7f) * pow(sunDot, 128.0f) * 1.5f;

    // 3. Infinite Ground Grid
    if (rd.y < -0.001f && ro.y > 0.0f) {
        float tGround = -ro.y / rd.y;
        if (tGround > 0.0f && tGround < 120.0f) {
            float3 hitGround = ro + rd * tGround;
            float2 gUV = hitGround.xz;

            float2 gMinor = abs(frac(gUV - 0.5f) - 0.5f) / max(fwidth(gUV), 0.001f);
            float minorGrid = 1.0f - min(min(gMinor.x, gMinor.y), 1.0f);

            float2 gMajorUV = gUV / 5.0f;
            float2 gMajor = abs(frac(gMajorUV - 0.5f) - 0.5f) / max(fwidth(gMajorUV), 0.001f);
            float majorGrid = 1.0f - min(min(gMajor.x, gMajor.y), 1.0f);

            float3 groundBase = float3(0.06f, 0.07f, 0.10f);
            float3 minorLineCol = float3(0.16f, 0.20f, 0.30f);
            float3 majorLineCol = float3(0.28f, 0.36f, 0.52f);

            float3 gCol = lerp(groundBase, minorLineCol, minorGrid * 0.6f);
            gCol = lerp(gCol, majorLineCol, majorGrid * 0.8f);

            float fog = clamp(tGround / 100.0f, 0.0f, 1.0f);
            sceneCol = lerp(gCol, skyHorizon, fog);
        }
    }

    // 4. Collectible 3D Floating Crystals (5 Locations)
    float3 crystalLocations[5] = {
        float3(  0.0f, 1.2f,  10.0f),
        float3( 12.0f, 1.2f,   4.0f),
        float3(-10.0f, 1.2f,  -6.0f),
        float3( 14.0f, 1.2f,  16.0f),
        float3(-14.0f, 1.2f,  10.0f)
    };

    float3 crystalColors[5] = {
        float3(0.2f, 0.9f, 0.4f), // Emerald
        float3(0.1f, 0.8f, 1.0f), // Cyan
        float3(0.8f, 0.3f, 1.0f), // Amethyst
        float3(1.0f, 0.8f, 0.1f), // Gold
        float3(1.0f, 0.2f, 0.3f)  // Ruby
    };

    uint mask = uint(CrystalMask);

    for (int i = 0; i < 5; ++i) {
        if ((mask & (1 << i)) == 0) { // Not yet collected!
            float3 cPos = crystalLocations[i];
            cPos.y += sin(Time * 2.5f + float(i) * 1.2f) * 0.25f;

            float tC = 0.0f;
            if (RaySphereIntersect(ro, rd, cPos, 0.65f, tC)) {
                float3 hitP = ro + rd * tC;
                float3 n = normalize(hitP - cPos);
                float diff = max(dot(n, sunDir), 0.0f);
                float spec = pow(max(dot(n, normalize(sunDir - rd)), 0.0f), 32.0f);

                float3 crystalCol = crystalColors[i] * (0.4f + diff * 0.8f) + float3(1, 1, 1) * spec;
                // Glowing pulsating crystal core
                crystalCol += crystalColors[i] * (0.6f + sin(Time * 4.0f) * 0.3f);
                sceneCol = crystalCol;
            }
        }
    }

    // 5. Player Controlled 3D Avatar (Rover / Physics Cube)
    float3 pBoxMin = playerPos - float3(0.7f, 0.0f, 0.7f);
    float3 pBoxMax = playerPos + float3(0.7f, 1.2f, 0.7f);
    float tPlayer = 0.0f;
    float3 pNormal = float3(0, 1, 0);

    if (RayBoxIntersect(ro, rd, pBoxMin, pBoxMax, tPlayer, pNormal)) {
        float diff = max(dot(pNormal, sunDir), 0.0f);
        float spec = pow(max(dot(pNormal, normalize(sunDir - rd)), 0.0f), 32.0f);

        // Player: Sleek Cobalt Blue & Gold Alloy
        float3 pAlbedo = float3(0.20f, 0.45f, 0.90f);
        if (pNormal.y > 0.5f) pAlbedo = float3(0.95f, 0.75f, 0.25f);

        float3 pCol = pAlbedo * (0.2f + diff * 0.9f) + float3(1, 1, 1) * spec * 0.6f;

        // Front headlights glow
        if (pNormal.z > 0.5f) {
            pCol += float3(0.3f, 0.8f, 1.0f) * 0.8f;
        }

        sceneCol = pCol;
    }

    // =========================================================================
    // 6. HEADS-UP DISPLAY (HUD) & GAMEPLAY OVERLAYS
    // =========================================================================
    // Top-Left Banner Background
    if (px.x >= 20.0f && px.x <= 340.0f && px.y >= 20.0f && px.y <= 95.0f) {
        sceneCol = lerp(sceneCol, float3(0.08f, 0.10f, 0.15f), 0.85f);
    }

    // Text: CRYSTAL SEEKER 3D
    if (Str7(px, float2(30.0f, 30.0f), 'C', 'R', 'Y', 'S', 'T', 'A', 'L', 1.8f)) sceneCol = float3(0.3f, 0.9f, 1.0f);

    // Text: SCORE
    if (Str5(px, float2(30.0f, 55.0f), 'S', 'C', 'O', 'R', 'E', 1.6f)) sceneCol = float3(1.0f, 0.9f, 0.3f);

    // Display Collected Crystals Counter
    uint collectedCount = 0;
    for (int k = 0; k < 5; ++k) {
        if ((mask & (1 << k)) != 0) collectedCount++;
    }
    uint numChar = '0' + collectedCount;
    if (IsChar(px, float2(120.0f, 55.0f), numChar, 1.6f)) sceneCol = float3(1, 1, 1);
    if (IsChar(px, float2(130.0f, 55.0f), '/', 1.6f)) sceneCol = float3(0.7f, 0.7f, 0.7f);
    if (IsChar(px, float2(140.0f, 55.0f), '5', 1.6f)) sceneCol = float3(0.4f, 1.0f, 0.4f);

    // Center Crosshair
    float2 centerDist = abs(px - float2(w * 0.5f, h * 0.5f));
    if ((centerDist.x < 10.0f && centerDist.y < 2.0f) || (centerDist.y < 10.0f && centerDist.x < 2.0f)) {
        sceneCol = lerp(sceneCol, float3(1.0f, 1.0f, 1.0f), 0.8f);
    }

    // Bottom Controls Banner
    if (px.y >= h - 35.0f) {
        sceneCol = lerp(sceneCol, float3(0.06f, 0.07f, 0.10f), 0.8f);
        // Hint text: WASD DRIVE
        if (Str4(px, float2(30.0f, h - 24.0f), 'W', 'A', 'S', 'D', 1.5f)) sceneCol = float3(0.4f, 0.8f, 1.0f);
        // SPACE JUMP
        if (Str4(px, float2(120.0f, h - 24.0f), 'J', 'U', 'M', 'P', 1.5f)) sceneCol = float3(0.8f, 0.8f, 0.9f);
        // REPLAY
        if (Str6(px, float2(220.0f, h - 24.0f), 'R', 'E', 'S', 'E', 'T', ' ', 1.5f)) sceneCol = float3(0.9f, 0.5f, 0.3f);
    }

    // VICTORY BANNER (when all 5 crystals collected)
    if (GameWon > 0.5f) {
        float2 bMin = float2(w * 0.5f - 180.0f, h * 0.5f - 60.0f);
        float2 bMax = float2(w * 0.5f + 180.0f, h * 0.5f + 40.0f);
        if (px.x >= bMin.x && px.x <= bMax.x && px.y >= bMin.y && px.y <= bMax.y) {
            sceneCol = lerp(sceneCol, float3(0.1f, 0.3f, 0.18f), 0.9f);
        }
        // Text: VICTORY!
        if (Str8(px, float2(w * 0.5f - 70.0f, h * 0.5f - 30.0f), 'V', 'I', 'C', 'T', 'O', 'R', 'Y', '!', 2.5f)) {
            sceneCol = float3(1.0f, 0.95f, 0.2f);
        }
    }

    return float4(sceneCol, 1.0f);
}