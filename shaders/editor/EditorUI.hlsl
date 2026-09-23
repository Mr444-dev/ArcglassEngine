// Modern 3D Game Engine - Complete AAA Production Editor Workspace
// Direct3D 12 SM 5.1 / 6.0 Compatible

cbuffer PushConstants : register(b0) {
    float Time;
    float ScreenWidth;
    float ScreenHeight;
    float Param0; // vpCamYaw
    float Param1; // vpCamPitch
    float Param2; // simulationState (0=EDIT, 1=PLAY, 2=PAUSE)
    float Param3; // selectedEntity (0..7)
    float Param4; // activeMenu (0=None, 1=File, 2=Edit, 3=Scene, 4=Build)
    float Param5; // bottomTab (0=Assets, 1=Console)
    float Param6; // activeModal (0=None, 1=Settings, 2=BuildModal, 3=AddComponentModal)
    float Param7; // entityCount (1..8)
    float Param8; // camPosX
    float Param9; // camPosY
    float Param10;// camPosZ
    float Param11;// gizmoMode (0=Select, 1=Move, 2=Rotate, 3=Scale)
    float Param12;// debugDraw (0=OFF, 1=ON Wireframes & Vectors)

    // 8 Active Scene Entities: (posX, posY, posZ, type: 0=Cube, 1=Sphere, 2=Cylinder, 3=Plane, 4=Light)
    float4 Entity0;
    float4 Entity1;
    float4 Entity2;
    float4 Entity3;
    float4 Entity4;
    float4 Entity5;
    float4 Entity6;
    float4 Entity7;
};

struct VSOutput {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

VSOutput VSMain(uint vertexID : SV_VertexID) {
    VSOutput output;
    float2 uvs[3] = { float2(0.0f, 0.0f), float2(2.0f, 0.0f), float2(0.0f, 2.0f) };
    output.uv = uvs[vertexID % 3];
    output.position = float4(output.uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    return output;
}

static const uint FontGlyphs[128] = {
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x404444, 0x000000, 0x000000, 0x000000, 0x924490, 0x000000, 0x000000,
    0x244442, 0x422224, 0x000000, 0x04E400, 0x000000, 0x00F000, 0x660000, 0x124880,
    0x699996, 0xE44464, 0xF8421F, 0xE1161F, 0x11F999, 0xF11E8F, 0x699E86, 0x22211F,
    0x699696, 0x611F96, 0x060600, 0x000000, 0x248421, 0x0F0F00, 0x842124, 0x402126,
    0x000000, 0x999F96, 0xE99E9E, 0x788887, 0xE9999E, 0xF88E8F, 0x888E8F, 0x799B87,
    0x999F99, 0xE4444E, 0x699111, 0x9ACCAC, 0xF88888, 0x9999F9, 0x999BD9, 0x699996,
    0x888E9E, 0x599996, 0x99AE9E, 0x691687, 0x66666F, 0x699999, 0x669999, 0x9F9999,
    0x966669, 0x666699, 0xF8421F, 0xE8888E, 0x000000, 0xE2222E, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000
};


bool IsChar(float2 px, float2 pos, uint c, float scale) {
    float2 rel = (px - pos) / scale;
    if (rel.x < 0.0f || rel.x >= 4.0f || rel.y < 0.0f || rel.y >= 6.0f) return false;
    uint x = 3 - uint(rel.x);
    uint y = uint(rel.y);
    return ((FontGlyphs[c & 127] >> (y * 4 + x)) & 1) != 0;
}

bool Str3(float2 px, float2 pos, uint c0, uint c1, uint c2, float s = 1.6f) {
    float w = 5.0f * s;
    return IsChar(px, pos, c0, s) || IsChar(px, pos + float2(w, 0), c1, s) || IsChar(px, pos + float2(2*w, 0), c2, s);
}

bool Str4(float2 px, float2 pos, uint c0, uint c1, uint c2, uint c3, float s = 1.6f) {
    float w = 5.0f * s;
    return IsChar(px, pos, c0, s) || IsChar(px, pos + float2(w, 0), c1, s) || IsChar(px, pos + float2(2*w, 0), c2, s) || IsChar(px, pos + float2(3*w, 0), c3, s);
}

bool Str5(float2 px, float2 pos, uint c0, uint c1, uint c2, uint c3, uint c4, float s = 1.6f) {
    float w = 5.0f * s;
    return IsChar(px, pos, c0, s) || IsChar(px, pos + float2(w, 0), c1, s) || IsChar(px, pos + float2(2*w, 0), c2, s) || IsChar(px, pos + float2(3*w, 0), c3, s) || IsChar(px, pos + float2(4*w, 0), c4, s);
}

bool Str6(float2 px, float2 pos, uint c0, uint c1, uint c2, uint c3, uint c4, uint c5, float s = 1.6f) {
    float w = 5.0f * s;
    return Str3(px, pos, c0, c1, c2, s) || Str3(px, pos + float2(3*w, 0), c3, c4, c5, s);
}

bool Str7(float2 px, float2 pos, uint c0, uint c1, uint c2, uint c3, uint c4, uint c5, uint c6, float s = 1.6f) {
    float w = 5.0f * s;
    return Str4(px, pos, c0, c1, c2, c3, s) || Str3(px, pos + float2(4*w, 0), c4, c5, c6, s);
}

bool Str8(float2 px, float2 pos, uint c0, uint c1, uint c2, uint c3, uint c4, uint c5, uint c6, uint c7, float s = 1.6f) {
    float w = 5.0f * s;
    return Str4(px, pos, c0, c1, c2, c3, s) || Str4(px, pos + float2(4*w, 0), c4, c5, c6, c7, s);
}

bool Str9(float2 px, float2 pos, uint c0, uint c1, uint c2, uint c3, uint c4, uint c5, uint c6, uint c7, uint c8, float s = 1.6f) {
    float w = 5.0f * s;
    return Str5(px, pos, c0, c1, c2, c3, c4, s) || Str4(px, pos + float2(5*w, 0), c5, c6, c7, c8, s);
}

bool Str10(float2 px, float2 pos, uint c0, uint c1, uint c2, uint c3, uint c4, uint c5, uint c6, uint c7, uint c8, uint c9, float s = 1.6f) {
    float w = 5.0f * s;
    return Str5(px, pos, c0, c1, c2, c3, c4, s) || Str5(px, pos + float2(5*w, 0), c5, c6, c7, c8, c9, s);
}

// 3D Geometric Primitive Ray Intersection Routines

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

bool RaySphereIntersect(float3 ro, float3 rd, float3 center, float radius, out float tNear, out float3 normal) {
    float3 oc = ro - center;
    float b = dot(oc, rd);
    float c = dot(oc, oc) - radius * radius;
    float h = b * b - c;
    if (h < 0.0f) {
        tNear = 1e9;
        normal = float3(0, 1, 0);
        return false;
    }
    h = sqrt(h);
    tNear = -b - h;
    if (tNear < 0.0f) tNear = -b + h;
    if (tNear < 0.0f) {
        tNear = 1e9;
        normal = float3(0, 1, 0);
        return false;
    }
    normal = normalize(ro + rd * tNear - center);
    return true;
}

bool RayCylinderIntersect(float3 ro, float3 rd, float3 center, float radius, float halfH, out float tNear, out float3 normal) {
    float3 roL = ro - center;
    float a = rd.x * rd.x + rd.z * rd.z;
    float b = 2.0f * (roL.x * rd.x + roL.z * rd.z);
    float c = roL.x * roL.x + roL.z * roL.z - radius * radius;
    float disc = b * b - 4.0f * a * c;

    if (disc >= 0.0f && a > 0.0001f) {
        float sqrtD = sqrt(disc);
        float t = (-b - sqrtD) / (2.0f * a);
        if (t < 0.0f) t = (-b + sqrtD) / (2.0f * a);
        if (t > 0.0f) {
            float yHit = roL.y + rd.y * t;
            if (abs(yHit) <= halfH) {
                tNear = t;
                normal = normalize(float3(roL.x + rd.x * t, 0.0f, roL.z + rd.z * t));
                return true;
            }
        }
    }
    tNear = 1e9;
    normal = float3(0, 1, 0);
    return false;
}

bool RayPlaneIntersect(float3 ro, float3 rd, float3 center, float2 halfSize, out float tNear, out float3 normal) {
    if (abs(rd.y) < 0.0001f) { tNear = 1e9; normal = float3(0,1,0); return false; }
    float t = (center.y - ro.y) / rd.y;
    if (t <= 0.0f) { tNear = 1e9; normal = float3(0,1,0); return false; }
    float3 hit = ro + rd * t;
    if (abs(hit.x - center.x) <= halfSize.x && abs(hit.z - center.z) <= halfSize.y) {
        tNear = t;
        normal = float3(0.0f, 1.0f, 0.0f);
        return true;
    }
    tNear = 1e9;
    normal = float3(0,1,0);
    return false;
}

float4 PSMain(VSOutput input) : SV_Target {
    float2 px = input.position.xy;
    float w = ScreenWidth;
    float h = ScreenHeight;

    const float topBarH = 74.0f;     // Unreal/Unity Toolbar + Action Palette
    const float leftDockW = 270.0f;   // Outliner / Hierarchy
    const float rightDockW = 330.0f;  // Details / Inspector
    const float bottomDockH = 200.0f; // Content Browser & Output Log

    float3 finalColor = float3(0.08f, 0.09f, 0.12f);

    // =========================================================================
    // 1. CENTRAL 3D SCENE VIEWPORT (WASD FREE-FLY CAMERA & 3D GIZMO)
    // =========================================================================
    float2 vpMin = float2(leftDockW, topBarH);
    float2 vpMax = float2(w - rightDockW, h - bottomDockH);

    if (px.x >= vpMin.x && px.x <= vpMax.x && px.y >= vpMin.y && px.y <= vpMax.y) {
        float2 vpCenter = (vpMin + vpMax) * 0.5f;
        float2 vpSize = vpMax - vpMin;
        float2 uv = (px - vpCenter) / vpSize.y * 2.0f;

        // Unreal/Unity Free-Fly Camera
        float3 ro = float3(Param8, Param9, Param10);
        float yaw = Param0;
        float pitch = Param1;

        float3 ww = normalize(float3(sin(yaw) * cos(pitch), sin(pitch), cos(yaw) * cos(pitch)));
        float3 uu = normalize(cross(ww, float3(0.0f, 1.0f, 0.0f)));
        float3 vv = normalize(cross(uu, ww));

        float3 rd = normalize(uv.x * uu * 0.70f - uv.y * vv * 0.70f + ww);

        // Sky atmosphere gradient
        float3 skyCol = lerp(float3(0.08f, 0.11f, 0.18f), float3(0.02f, 0.03f, 0.06f), clamp(rd.y * 0.5f + 0.5f, 0.0f, 1.0f));
        float3 sceneCol = skyCol;

        // Ground Infinite Grid (Y = 0)
        if (rd.y < -0.001f && ro.y > 0.0f) {
            float tGround = -ro.y / rd.y;
            if (tGround > 0.0f && tGround < 70.0f) {
                float3 hitGround = ro + rd * tGround;
                float2 gUV = hitGround.xz;
                float2 gLines = abs(frac(gUV - 0.5f) - 0.5f) / max(fwidth(gUV), 0.001f);
                float lineVal = min(gLines.x, gLines.y);
                float gridIntensity = 1.0f - min(lineVal, 1.0f);

                float axisX = 1.0f - min(abs(hitGround.z) / max(fwidth(hitGround.z), 0.001f), 1.0f);
                float axisZ = 1.0f - min(abs(hitGround.x) / max(fwidth(hitGround.x), 0.001f), 1.0f);

                float3 baseGround = float3(0.07f, 0.08f, 0.10f);
                float3 lineCol = float3(0.18f, 0.22f, 0.30f);
                float3 gCol = lerp(baseGround, lineCol, gridIntensity * 0.8f);

                gCol = lerp(gCol, float3(0.95f, 0.25f, 0.25f), axisX * (hitGround.x > 0.0f ? 0.95f : 0.4f)); // X Red
                gCol = lerp(gCol, float3(0.25f, 0.55f, 1.0f), axisZ * (hitGround.z > 0.0f ? 0.95f : 0.4f)); // Z Blue

                float fogFactor = clamp(tGround / 65.0f, 0.0f, 1.0f);
                sceneCol = lerp(gCol, skyCol, fogFactor);
            }
        }

        // Raycast against ALL active scene entities
        int numEntities = min(int(Param7), 8);
        float tClosest = 1e9;
        float3 hitNormal = float3(0, 1, 0);
        int hitEntity = -1;
        float3 hitCenter = float3(0, 0, 0);
        int hitType = 0;

        for (int i = 0; i < numEntities; ++i) {
            float4 ent = (i == 0) ? Entity0 :
                         (i == 1) ? Entity1 :
                         (i == 2) ? Entity2 :
                         (i == 3) ? Entity3 :
                         (i == 4) ? Entity4 :
                         (i == 5) ? Entity5 :
                         (i == 6) ? Entity6 : Entity7;

            float3 pos = ent.xyz;
            int type = int(ent.w);
            float tHit = 1e9;
            float3 nHit = float3(0, 1, 0);

            if (type == 0) { // Cube
                float3 bMin = pos - float3(0.55f, 0.55f, 0.55f);
                float3 bMax = pos + float3(0.55f, 0.55f, 0.55f);
                if (RayBoxIntersect(ro, rd, bMin, bMax, tHit, nHit)) {
                    if (tHit < tClosest) { tClosest = tHit; hitNormal = nHit; hitEntity = i; hitCenter = pos; hitType = 0; }
                }
            } else if (type == 1) { // Sphere
                if (RaySphereIntersect(ro, rd, pos, 0.60f, tHit, nHit)) {
                    if (tHit < tClosest) { tClosest = tHit; hitNormal = nHit; hitEntity = i; hitCenter = pos; hitType = 1; }
                }
            } else if (type == 2) { // Cylinder
                if (RayCylinderIntersect(ro, rd, pos, 0.50f, 0.70f, tHit, nHit)) {
                    if (tHit < tClosest) { tClosest = tHit; hitNormal = nHit; hitEntity = i; hitCenter = pos; hitType = 2; }
                }
            } else if (type == 3) { // Plane
                if (RayPlaneIntersect(ro, rd, pos, float2(1.5f, 1.5f), tHit, nHit)) {
                    if (tHit < tClosest) { tClosest = tHit; hitNormal = nHit; hitEntity = i; hitCenter = pos; hitType = 3; }
                }
            } else if (type == 4) { // Light Marker
                float3 bMin = pos - float3(0.25f, 0.25f, 0.25f);
                float3 bMax = pos + float3(0.25f, 0.25f, 0.25f);
                if (RayBoxIntersect(ro, rd, bMin, bMax, tHit, nHit)) {
                    if (tHit < tClosest) { tClosest = tHit; hitNormal = nHit; hitEntity = i; hitCenter = pos; hitType = 4; }
                }
            }
        }

        // If entity hit, shade with PBR lighting
        if (hitEntity >= 0) {
            float3 lightDir = normalize(float3(0.5f, 0.8f, 0.35f));
            float NdotL = max(dot(hitNormal, lightDir), 0.0f);
            float3 viewDir = -rd;
            float3 halfDir = normalize(lightDir + viewDir);
            float spec = pow(max(dot(hitNormal, halfDir), 0.0f), 32.0f);

            float3 albedo = float3(0.85f, 0.45f, 0.25f); // Cube (Orange)
            if (hitType == 1) albedo = float3(0.25f, 0.65f, 0.95f); // Sphere (Cyan)
            else if (hitType == 2) albedo = float3(0.40f, 0.85f, 0.45f); // Cylinder (Emerald)
            else if (hitType == 3) {
                float2 uvP = (ro + rd * tClosest).xz - hitCenter.xz;
                float chk = step(0.5f, frac(uvP.x * 2.0f)) == step(0.5f, frac(uvP.y * 2.0f)) ? 0.7f : 0.4f;
                albedo = float3(chk, chk, chk);
            } else if (hitType == 4) albedo = float3(1.0f, 0.9f, 0.25f); // Light

            float3 ambient = float3(0.08f, 0.09f, 0.12f) * albedo;
            float3 diffuse = albedo * NdotL * 1.2f;
            float3 specular = float3(1, 1, 1) * spec * 0.5f;
            float3 objCol = ambient + diffuse + specular;

            // Selected Entity Glowing Outline (Unreal Yellow)
            if (hitEntity == int(Param3)) {
                float rim = 1.0f - max(dot(hitNormal, viewDir), 0.0f);
                if (rim > 0.65f) objCol = float3(1.0f, 0.75f, 0.0f);
            }

            // Debug Drawing (F5): Wireframe collider bounds in bright green!
            if (Param12 > 0.5f) {
                float3 pLocal = (ro + rd * tClosest) - hitCenter;
                float3 dLocal = abs(pLocal);
                if (abs(dLocal.x - 0.55f) < 0.03f || abs(dLocal.y - 0.55f) < 0.03f || abs(dLocal.z - 0.55f) < 0.03f) {
                    objCol = float3(0.1f, 1.0f, 0.2f); // Fluorescent Green Collider Wireframe
                }
            }

            sceneCol = objCol;
        }

        // 3D TRANSFORM GIZMO on Selected Entity
        int selId = int(Param3);
        if (selId >= 0 && selId < numEntities) {
            float4 selEnt = (selId == 0) ? Entity0 :
                            (selId == 1) ? Entity1 :
                            (selId == 2) ? Entity2 :
                            (selId == 3) ? Entity3 :
                            (selId == 4) ? Entity4 :
                            (selId == 5) ? Entity5 :
                            (selId == 6) ? Entity6 : Entity7;
            float3 gPos = selEnt.xyz;

            // X Axis Handle (Red)
            float tGizX = 1e9; float3 nGizX = float3(0,1,0);
            if (RayBoxIntersect(ro, rd, gPos + float3(0.1f, -0.04f, -0.04f), gPos + float3(1.2f, 0.04f, 0.04f), tGizX, nGizX)) {
                if (tGizX < tClosest) sceneCol = float3(1.0f, 0.2f, 0.2f);
            }
            // Y Axis Handle (Green)
            float tGizY = 1e9; float3 nGizY = float3(0,1,0);
            if (RayBoxIntersect(ro, rd, gPos + float3(-0.04f, 0.1f, -0.04f), gPos + float3(0.04f, 1.2f, 0.04f), tGizY, nGizY)) {
                if (tGizY < tClosest) sceneCol = float3(0.2f, 1.0f, 0.3f);
            }
            // Z Axis Handle (Blue)
            float tGizZ = 1e9; float3 nGizZ = float3(0,1,0);
            if (RayBoxIntersect(ro, rd, gPos + float3(-0.04f, -0.04f, 0.1f), gPos + float3(0.04f, 0.04f, 1.2f), tGizZ, nGizZ)) {
                if (tGizZ < tClosest) sceneCol = float3(0.2f, 0.55f, 1.0f);
            }
        }

        // Viewport Header Bar with Gizmo modes & simulation state
        if (px.y >= vpMin.y && px.y <= vpMin.y + 26.0f) {
            sceneCol = lerp(sceneCol, float3(0.11f, 0.12f, 0.16f), 0.94f);

            int gMode = int(Param11);
            if (px.x >= vpMin.x + 8.0f && px.x <= vpMin.x + 32.0f && px.y >= vpMin.y + 3.0f && px.y <= vpMin.y + 23.0f) {
                sceneCol = (gMode == 0) ? float3(0.20f, 0.45f, 0.85f) : float3(0.16f, 0.18f, 0.24f);
            }
            if (IsChar(px, float2(vpMin.x + 16.0f, vpMin.y + 7.0f), 'Q', 1.4f)) sceneCol = float3(1, 1, 1);

            if (px.x >= vpMin.x + 36.0f && px.x <= vpMin.x + 60.0f && px.y >= vpMin.y + 3.0f && px.y <= vpMin.y + 23.0f) {
                sceneCol = (gMode == 1) ? float3(0.20f, 0.45f, 0.85f) : float3(0.16f, 0.18f, 0.24f);
            }
            if (IsChar(px, float2(vpMin.x + 44.0f, vpMin.y + 7.0f), 'W', 1.4f)) sceneCol = float3(1, 1, 1);

            if (px.x >= vpMin.x + 64.0f && px.x <= vpMin.x + 88.0f && px.y >= vpMin.y + 3.0f && px.y <= vpMin.y + 23.0f) {
                sceneCol = (gMode == 2) ? float3(0.20f, 0.45f, 0.85f) : float3(0.16f, 0.18f, 0.24f);
            }
            if (IsChar(px, float2(vpMin.x + 72.0f, vpMin.y + 7.0f), 'E', 1.4f)) sceneCol = float3(1, 1, 1);

            if (px.x >= vpMin.x + 92.0f && px.x <= vpMin.x + 116.0f && px.y >= vpMin.y + 3.0f && px.y <= vpMin.y + 23.0f) {
                sceneCol = (gMode == 3) ? float3(0.20f, 0.45f, 0.85f) : float3(0.16f, 0.18f, 0.24f);
            }
            if (IsChar(px, float2(vpMin.x + 100.0f, vpMin.y + 7.0f), 'R', 1.4f)) sceneCol = float3(1, 1, 1);

            if (Str8(px, float2(vpMin.x + 135.0f, vpMin.y + 7.0f), 'P', 'E', 'R', 'S', 'P', 'E', 'C', 'T', 1.4f)) sceneCol = float3(0.65f, 0.7f, 0.85f);
            if (Str4(px, float2(vpMin.x + 205.0f, vpMin.y + 7.0f), 'L', 'I', 'T', ' ', 1.4f)) sceneCol = float3(0.4f, 0.9f, 0.6f);

            // Simulation state badge
            int simState = int(Param2);
            if (simState == 1) {
                if (px.x >= vpMax.x - 90.0f && px.x <= vpMax.x - 10.0f && px.y >= vpMin.y + 4.0f && px.y <= vpMin.y + 22.0f) sceneCol = float3(0.18f, 0.65f, 0.35f);
                if (Str7(px, float2(vpMax.x - 80.0f, vpMin.y + 7.0f), 'P', 'L', 'A', 'Y', 'I', 'N', 'G', 1.4f)) sceneCol = float3(1, 1, 1);
            } else if (simState == 2) {
                if (px.x >= vpMax.x - 85.0f && px.x <= vpMax.x - 10.0f && px.y >= vpMin.y + 4.0f && px.y <= vpMin.y + 22.0f) sceneCol = float3(0.70f, 0.55f, 0.15f);
                if (Str6(px, float2(vpMax.x - 75.0f, vpMin.y + 7.0f), 'P', 'A', 'U', 'S', 'E', 'D', 1.4f)) sceneCol = float3(1, 1, 1);
            } else {
                if (px.x >= vpMax.x - 80.0f && px.x <= vpMax.x - 10.0f && px.y >= vpMin.y + 4.0f && px.y <= vpMin.y + 22.0f) sceneCol = float3(0.18f, 0.30f, 0.50f);
                if (Str4(px, float2(vpMax.x - 65.0f, vpMin.y + 7.0f), 'E', 'D', 'I', 'T', 1.4f)) sceneCol = float3(1, 1, 1);
            }
        }

        if (abs(px.x - vpMin.x) < 1.0f || abs(px.x - vpMax.x) < 1.0f || abs(px.y - vpMin.y) < 1.0f || abs(px.y - vpMax.y) < 1.0f) {
            sceneCol = float3(0.20f, 0.22f, 0.30f);
        }
        finalColor = sceneCol;
    }

    // =========================================================================
    // 2. UNREAL/UNITY TOP TOOLBAR & SIMULATION CONTROLS
    // =========================================================================
    if (px.y < topBarH) {
        float3 barCol = float3(0.10f, 0.11f, 0.15f);

        // Menu Bar Row (Y: 0 to 32)
        if (px.y < 32.0f) {
            if (px.x < 75.0f) barCol = float3(0.16f, 0.22f, 0.34f);

            int menu = int(Param4);
            if (menu == 1 && px.x >= 80.0f && px.x <= 130.0f) barCol = float3(0.22f, 0.26f, 0.38f);
            if (menu == 2 && px.x >= 130.0f && px.x <= 180.0f) barCol = float3(0.22f, 0.26f, 0.38f);
            if (menu == 3 && px.x >= 180.0f && px.x <= 240.0f) barCol = float3(0.22f, 0.26f, 0.38f);
            if (menu == 4 && px.x >= 240.0f && px.x <= 305.0f) barCol = float3(0.22f, 0.26f, 0.38f);

            float centerX = w * 0.5f;
            int simState = int(Param2);

            // [▶ PLAY]
            if (px.x >= centerX - 120.0f && px.x <= centerX - 65.0f && px.y >= 5.0f && px.y <= 27.0f) {
                barCol = (simState == 1) ? float3(0.24f, 0.78f, 0.40f) : float3(0.16f, 0.50f, 0.28f);
            }
            // [⏸ PAUSE]
            else if (px.x >= centerX - 60.0f && px.x <= centerX - 10.0f && px.y >= 5.0f && px.y <= 27.0f) {
                barCol = (simState == 2) ? float3(0.75f, 0.55f, 0.18f) : float3(0.18f, 0.20f, 0.28f);
            }
            // [⏭ STEP]
            else if (px.x >= centerX - 5.0f && px.x <= centerX + 45.0f && px.y >= 5.0f && px.y <= 27.0f) {
                barCol = float3(0.20f, 0.35f, 0.55f);
            }
            // [⏹ STOP]
            else if (px.x >= centerX + 50.0f && px.x <= centerX + 100.0f && px.y >= 5.0f && px.y <= 27.0f) {
                barCol = float3(0.68f, 0.20f, 0.22f);
            }

            if (Str6(px, float2(12.0f, 8.0f), 'E', 'N', 'G', 'I', 'N', 'E', 1.6f)) barCol = float3(0.4f, 0.8f, 1.0f);
            if (Str4(px, float2(90.0f, 8.0f), 'F', 'I', 'L', 'E', 1.6f)) barCol = float3(0.95f, 0.95f, 1.0f);
            if (Str4(px, float2(140.0f, 8.0f), 'E', 'D', 'I', 'T', 1.6f)) barCol = float3(0.95f, 0.95f, 1.0f);
            if (Str5(px, float2(190.0f, 8.0f), 'S', 'C', 'E', 'N', 'E', 1.6f)) barCol = float3(0.95f, 0.95f, 1.0f);
            if (Str5(px, float2(250.0f, 8.0f), 'B', 'U', 'I', 'L', 'D', 1.6f)) barCol = float3(0.95f, 0.95f, 1.0f);

            if (Str4(px, float2(centerX - 108.0f, 8.0f), 'P', 'L', 'A', 'Y', 1.5f)) barCol = float3(1, 1, 1);
            if (Str5(px, float2(centerX - 52.0f, 8.0f), 'P', 'A', 'U', 'S', 1.4f)) barCol = float3(1, 1, 1);
            if (Str4(px, float2(centerX + 5.0f, 8.0f), 'S', 'T', 'E', 'P', 1.5f)) barCol = float3(1, 1, 1);
            if (Str4(px, float2(centerX + 60.0f, 8.0f), 'S', 'T', 'O', 'P', 1.5f)) barCol = float3(1, 1, 1);

            // [PACKAGE / BUILD GAME] Toolbar Button
            if (px.x >= w - 150.0f && px.x <= w - 15.0f && px.y >= 5.0f && px.y <= 27.0f) barCol = float3(0.18f, 0.45f, 0.75f);
            if (Str10(px, float2(w - 142.0f, 8.0f), 'B', 'U', 'I', 'L', 'D', ' ', 'G', 'A', 'M', 'E', 1.4f)) barCol = float3(1, 1, 1);
        }
        // Quick Object Creation Row (Y: 33 to 74)
        else {
            barCol = float3(0.12f, 0.13f, 0.18f);

            if (px.x >= 12.0f && px.x <= 82.0f && px.y >= 38.0f && px.y <= 68.0f) barCol = float3(0.22f, 0.28f, 0.42f);
            if (Str6(px, float2(18.0f, 48.0f), '+', ' ', 'C', 'U', 'B', 'E', 1.5f)) barCol = float3(1, 1, 1);

            if (px.x >= 88.0f && px.x <= 170.0f && px.y >= 38.0f && px.y <= 68.0f) barCol = float3(0.22f, 0.28f, 0.42f);
            if (Str8(px, float2(94.0f, 48.0f), '+', ' ', 'S', 'P', 'H', 'E', 'R', 'E', 1.5f)) barCol = float3(1, 1, 1);

            if (px.x >= 176.0f && px.x <= 265.0f && px.y >= 38.0f && px.y <= 68.0f) barCol = float3(0.22f, 0.28f, 0.42f);
            if (Str8(px, float2(182.0f, 48.0f), '+', ' ', 'C', 'Y', 'L', 'I', 'N', 'D', 1.4f)) barCol = float3(1, 1, 1);

            if (px.x >= 271.0f && px.x <= 348.0f && px.y >= 38.0f && px.y <= 68.0f) barCol = float3(0.22f, 0.28f, 0.42f);
            if (Str7(px, float2(277.0f, 48.0f), '+', ' ', 'P', 'L', 'A', 'N', 'E', 1.5f)) barCol = float3(1, 1, 1);

            if (px.x >= 354.0f && px.x <= 430.0f && px.y >= 38.0f && px.y <= 68.0f) barCol = float3(0.28f, 0.26f, 0.16f);
            if (Str7(px, float2(360.0f, 48.0f), '+', ' ', 'L', 'I', 'G', 'H', 'T', 1.5f)) barCol = float3(1.0f, 0.9f, 0.3f);

            // [DEBUG DRAW F5]
            if (px.x >= 436.0f && px.x <= 525.0f && px.y >= 38.0f && px.y <= 68.0f) {
                barCol = (Param12 > 0.5f) ? float3(0.18f, 0.55f, 0.30f) : float3(0.18f, 0.20f, 0.28f);
            }
            if (Str8(px, float2(444.0f, 48.0f), 'D', 'E', 'B', 'U', 'G', ' ', 'F', '5', 1.4f)) barCol = float3(1, 1, 1);

            // [🗑 DELETE]
            if (px.x >= 531.0f && px.x <= 605.0f && px.y >= 38.0f && px.y <= 68.0f) barCol = float3(0.38f, 0.16f, 0.18f);
            if (Str6(px, float2(541.0f, 48.0f), 'D', 'E', 'L', 'E', 'T', 'E', 1.5f)) barCol = float3(1.0f, 0.5f, 0.5f);
        }

        if (px.y >= topBarH - 1.0f) barCol = float3(0.20f, 0.22f, 0.30f);
        finalColor = barCol;
    }

    // =========================================================================
    // 3. LEFT DOCK: OUTLINER / SCENE HIERARCHY
    // =========================================================================
    if (px.x < leftDockW && px.y >= topBarH && px.y < h - bottomDockH) {
        float3 panelCol = float3(0.09f, 0.10f, 0.13f);

        if (px.y < topBarH + 32.0f) {
            panelCol = float3(0.12f, 0.13f, 0.18f);
            if (px.y >= topBarH + 31.0f) panelCol = float3(0.20f, 0.22f, 0.30f);
            if (Str8(px, float2(15.0f, topBarH + 9.0f), 'O', 'U', 'T', 'L', 'I', 'N', 'E', 'R', 1.6f)) panelCol = float3(0.9f, 0.9f, 0.95f);
        } else {
            float rowY = px.y - (topBarH + 34.0f);
            int rowIndex = int(rowY / 30.0f);
            float inRowY = frac(rowY / 30.0f);

            int selId = int(Param3);
            int count = min(int(Param7), 8);

            if (rowIndex == selId && rowIndex < count) {
                panelCol = float3(0.18f, 0.35f, 0.65f);
                if (px.x <= 4.0f) panelCol = float3(1.0f, 0.75f, 0.0f); // Yellow accent
            } else if (rowIndex % 2 == 1 && rowIndex < count) {
                panelCol = float3(0.10f, 0.11f, 0.15f);
            }

            if (rowIndex < count) {
                float4 ent = (rowIndex == 0) ? Entity0 :
                             (rowIndex == 1) ? Entity1 :
                             (rowIndex == 2) ? Entity2 :
                             (rowIndex == 3) ? Entity3 :
                             (rowIndex == 4) ? Entity4 :
                             (rowIndex == 5) ? Entity5 :
                             (rowIndex == 6) ? Entity6 : Entity7;
                int type = int(ent.w);

                if (px.x >= 14.0f && px.x <= 26.0f && inRowY >= 0.25f && inRowY <= 0.75f) {
                    if (type == 0) panelCol = float3(0.85f, 0.45f, 0.25f);
                    else if (type == 1) panelCol = float3(0.25f, 0.65f, 0.95f);
                    else if (type == 2) panelCol = float3(0.40f, 0.85f, 0.45f);
                    else if (type == 3) panelCol = float3(0.70f, 0.70f, 0.70f);
                    else panelCol = float3(1.0f, 0.9f, 0.3f);
                }

                float itemY = topBarH + 42.0f + float(rowIndex) * 30.0f;
                if (type == 0 && Str4(px, float2(36.0f, itemY), 'C', 'U', 'B', 'E', 1.5f)) panelCol = float3(1, 1, 1);
                else if (type == 1 && Str6(px, float2(36.0f, itemY), 'S', 'P', 'H', 'E', 'R', 'E', 1.5f)) panelCol = float3(1, 1, 1);
                else if (type == 2 && Str8(px, float2(36.0f, itemY), 'C', 'Y', 'L', 'I', 'N', 'D', 'E', 'R', 1.4f)) panelCol = float3(1, 1, 1);
                else if (type == 3 && Str5(px, float2(36.0f, itemY), 'P', 'L', 'A', 'N', 'E', 1.5f)) panelCol = float3(1, 1, 1);
                else if (type == 4 && Str5(px, float2(36.0f, itemY), 'L', 'I', 'G', 'H', 'T', 1.5f)) panelCol = float3(1, 1, 1);
            }
        }

        if (px.x >= leftDockW - 1.0f) panelCol = float3(0.20f, 0.22f, 0.30f);
        finalColor = panelCol;
    }

    // =========================================================================
    // 4. RIGHT DOCK: DETAILS / INSPECTOR WITH '+ ADD COMPONENT'
    // =========================================================================
    if (px.x > w - rightDockW && px.y >= topBarH && px.y < h - bottomDockH) {
        float3 panelCol = float3(0.09f, 0.10f, 0.13f);
        int selId = int(Param3);

        if (px.y < topBarH + 32.0f) {
            panelCol = float3(0.12f, 0.13f, 0.18f);
            if (px.y >= topBarH + 31.0f) panelCol = float3(0.20f, 0.22f, 0.30f);
            if (Str7(px, float2(w - rightDockW + 15.0f, topBarH + 9.0f), 'D', 'E', 'T', 'A', 'I', 'L', 'S', 1.6f)) panelCol = float3(0.9f, 0.9f, 0.95f);
        } else {
            float relY = px.y - (topBarH + 36.0f);

            // SECTION 1: TRANSFORM
            if (relY >= 0.0f && relY <= 24.0f) {
                panelCol = float3(0.15f, 0.17f, 0.25f);
                if (Str9(px, float2(w - rightDockW + 15.0f, topBarH + 42.0f), 'T', 'R', 'A', 'N', 'S', 'F', 'O', 'R', 'M', 1.5f)) panelCol = float3(1, 1, 1);
            }
            // POS X / Y / Z
            else if (relY >= 30.0f && relY <= 110.0f) {
                float rowIdx = floor((relY - 30.0f) / 26.0f);
                float yRow = topBarH + 68.0f + rowIdx * 26.0f;

                if (rowIdx == 0 && Str5(px, float2(w - rightDockW + 15.0f, yRow), 'P', 'O', 'S', ' ', 'X', 1.4f)) panelCol = float3(0.95f, 0.4f, 0.4f);
                if (rowIdx == 1 && Str5(px, float2(w - rightDockW + 15.0f, yRow), 'P', 'O', 'S', ' ', 'Y', 1.4f)) panelCol = float3(0.4f, 0.95f, 0.4f);
                if (rowIdx == 2 && Str5(px, float2(w - rightDockW + 15.0f, yRow), 'P', 'O', 'S', ' ', 'Z', 1.4f)) panelCol = float3(0.4f, 0.65f, 1.0f);

                // [-]
                if (px.x >= w - rightDockW + 80.0f && px.x <= w - rightDockW + 105.0f && px.y >= yRow - 2.0f && px.y <= yRow + 16.0f) {
                    panelCol = float3(0.22f, 0.25f, 0.35f);
                    if (IsChar(px, float2(w - rightDockW + 90.0f, yRow), '-', 1.6f)) panelCol = float3(1, 1, 1);
                }
                // Numeric Box
                if (px.x >= w - rightDockW + 112.0f && px.x <= w - rightDockW + 225.0f && px.y >= yRow - 2.0f && px.y <= yRow + 16.0f) {
                    panelCol = float3(0.12f, 0.14f, 0.19f);
                    if (rowIdx == 0 && Str4(px, float2(w - rightDockW + 140.0f, yRow), '0', '.', '0', '0', 1.4f)) panelCol = float3(1, 1, 1);
                    if (rowIdx == 1 && Str4(px, float2(w - rightDockW + 140.0f, yRow), '0', '.', '8', '0', 1.4f)) panelCol = float3(1, 1, 1);
                    if (rowIdx == 2 && Str4(px, float2(w - rightDockW + 140.0f, yRow), '0', '.', '0', '0', 1.4f)) panelCol = float3(1, 1, 1);
                }
                // [+]
                if (px.x >= w - rightDockW + 232.0f && px.x <= w - rightDockW + 257.0f && px.y >= yRow - 2.0f && px.y <= yRow + 16.0f) {
                    panelCol = float3(0.22f, 0.25f, 0.35f);
                    if (IsChar(px, float2(w - rightDockW + 241.0f, yRow), '+', 1.6f)) panelCol = float3(1, 1, 1);
                }
            }
            // SECTION 2: MESH & MATERIAL
            else if (relY >= 120.0f && relY <= 144.0f) {
                panelCol = float3(0.15f, 0.17f, 0.25f);
                if (Str10(px, float2(w - rightDockW + 15.0f, topBarH + 162.0f), 'M', 'E', 'S', 'H', ' ', 'R', 'E', 'N', 'D', 'E', 1.4f)) panelCol = float3(1, 1, 1);
            } else if (relY >= 150.0f && relY <= 170.0f) {
                if (Str10(px, float2(w - rightDockW + 15.0f, topBarH + 192.0f), 'S', 'T', 'A', 'T', 'I', 'C', ' ', 'M', 'E', 'S', 1.4f)) panelCol = float3(0.85f, 0.85f, 0.9f);
            }
            // SECTION 3: RIGIDBODY PHYSICS
            else if (relY >= 180.0f && relY <= 204.0f) {
                panelCol = float3(0.15f, 0.17f, 0.25f);
                if (Str9(px, float2(w - rightDockW + 15.0f, topBarH + 222.0f), 'R', 'I', 'G', 'I', 'D', 'B', 'O', 'D', 'Y', 1.5f)) panelCol = float3(1, 1, 1);
            } else if (relY >= 210.0f && relY <= 230.0f) {
                if (Str10(px, float2(w - rightDockW + 15.0f, topBarH + 252.0f), 'G', 'R', 'A', 'V', 'I', 'T', 'Y', ':', ' ', 'O', 1.4f)) panelCol = float3(0.4f, 0.9f, 0.5f);
            }
            // BUTTON: [+ ADD COMPONENT] (Centered at bottom of inspector)
            else if (relY >= 250.0f && relY <= 280.0f) {
                float btnMinX = w - rightDockW + 20.0f;
                float btnMaxX = w - 20.0f;
                if (px.x >= btnMinX && px.x <= btnMaxX) {
                    panelCol = float3(0.18f, 0.35f, 0.65f); // Blue action button
                }
                if (Str10(px, float2(btnMinX + 35.0f, topBarH + 292.0f), '+', ' ', 'A', 'D', 'D', ' ', 'C', 'O', 'M', 'P', 1.4f)) panelCol = float3(1, 1, 1);
            }
        }

        if (px.x <= w - rightDockW + 1.0f) panelCol = float3(0.20f, 0.22f, 0.30f);
        finalColor = panelCol;
    }

    // =========================================================================
    // 5. BOTTOM DOCK: CONTENT BROWSER & CONSOLE
    // =========================================================================
    if (px.y >= h - bottomDockH) {
        float3 panelCol = float3(0.08f, 0.09f, 0.12f);
        int tab = int(Param5);

        if (px.y < h - bottomDockH + 28.0f) {
            panelCol = float3(0.11f, 0.12f, 0.16f);
            if (px.x >= 15.0f && px.x <= 150.0f) {
                panelCol = (tab == 0) ? float3(0.16f, 0.20f, 0.28f) : float3(0.11f, 0.12f, 0.16f);
                if (tab == 0 && px.y >= h - bottomDockH + 26.0f) panelCol = float3(1.0f, 0.75f, 0.0f);
            } else if (px.x >= 160.0f && px.x <= 260.0f) {
                panelCol = (tab == 1) ? float3(0.16f, 0.20f, 0.28f) : float3(0.11f, 0.12f, 0.16f);
                if (tab == 1 && px.y >= h - bottomDockH + 26.0f) panelCol = float3(1.0f, 0.75f, 0.0f);
            }

            if (Str7(px, float2(25.0f, h - bottomDockH + 8.0f), 'C', 'O', 'N', 'T', 'E', 'N', 'T', 1.5f)) panelCol = (tab == 0) ? float3(1, 1, 1) : float3(0.6f, 0.65f, 0.75f);
            if (Str7(px, float2(170.0f, h - bottomDockH + 8.0f), 'C', 'O', 'N', 'S', 'O', 'L', 'E', 1.5f)) panelCol = (tab == 1) ? float3(1, 1, 1) : float3(0.6f, 0.65f, 0.75f);
        } else {
            if (tab == 0) {
                // Left Categories
                if (px.x < 180.0f) {
                    panelCol = float3(0.09f, 0.10f, 0.14f);
                    float f0Y = h - bottomDockH + 36.0f;
                    if (Str6(px, float2(15.0f, f0Y + 6.0f), 'M', 'E', 'S', 'H', 'E', 'S', 1.4f)) panelCol = float3(1, 1, 1);
                    if (Str9(px, float2(15.0f, f0Y + 34.0f), 'M', 'A', 'T', 'E', 'R', 'I', 'A', 'L', 'S', 1.3f)) panelCol = float3(0.8f, 0.85f, 0.9f);
                    if (Str8(px, float2(15.0f, f0Y + 62.0f), 'T', 'E', 'X', 'T', 'U', 'R', 'E', 'S', 1.3f)) panelCol = float3(0.8f, 0.85f, 0.9f);
                    if (Str6(px, float2(15.0f, f0Y + 90.0f), 'S', 'C', 'E', 'N', 'E', 'S', 1.4f)) panelCol = float3(0.8f, 0.85f, 0.9f);
                    if (px.x >= 179.0f) panelCol = float3(0.20f, 0.22f, 0.30f);
                } else {
                    for (int c = 0; c < 5; ++c) {
                        float cMinX = 200.0f + c * 115.0f;
                        float cMaxX = cMinX + 100.0f;
                        float cMinY = h - bottomDockH + 38.0f;
                        float cMaxY = cMinY + 100.0f;

                        if (px.x >= cMinX && px.x <= cMaxX && px.y >= cMinY && px.y <= cMaxY) {
                            panelCol = float3(0.14f, 0.16f, 0.22f);
                            if (px.y <= cMinY + 65.0f) {
                                if (c == 0) panelCol = float3(0.85f, 0.45f, 0.25f);
                                else if (c == 1) panelCol = float3(0.25f, 0.65f, 0.95f);
                                else if (c == 2) panelCol = float3(0.40f, 0.85f, 0.45f);
                                else if (c == 3) panelCol = float3(0.70f, 0.70f, 0.70f);
                                else panelCol = float3(0.95f, 0.75f, 0.25f);
                            }
                        }
                        float labelY = cMinY + 74.0f;
                        if (c == 0 && Str4(px, float2(cMinX + 18.0f, labelY), 'C', 'U', 'B', 'E', 1.4f)) panelCol = float3(1, 1, 1);
                        else if (c == 1 && Str6(px, float2(cMinX + 12.0f, labelY), 'S', 'P', 'H', 'E', 'R', 'E', 1.3f)) panelCol = float3(1, 1, 1);
                        else if (c == 2 && Str6(px, float2(cMinX + 12.0f, labelY), 'C', 'Y', 'L', 'I', 'N', 'D', 1.3f)) panelCol = float3(1, 1, 1);
                        else if (c == 3 && Str5(px, float2(cMinX + 16.0f, labelY), 'P', 'L', 'A', 'N', 'E', 1.4f)) panelCol = float3(1, 1, 1);
                        else if (c == 4 && Str5(px, float2(cMinX + 16.0f, labelY), 'L', 'I', 'G', 'H', 'T', 1.4f)) panelCol = float3(1, 1, 1);
                    }
                }
            } else {
                // Console Output Log
                panelCol = float3(0.06f, 0.07f, 0.09f);
                float logStartY = h - bottomDockH + 34.0f;

                if (px.x >= 20.0f && px.x <= 60.0f && px.y >= logStartY && px.y <= logStartY + 14.0f) panelCol = float3(0.15f, 0.35f, 0.65f);
                if (Str4(px, float2(24.0f, logStartY + 2.0f), 'I', 'N', 'F', 'O', 1.3f)) panelCol = float3(1, 1, 1);
                if (Str10(px, float2(70.0f, logStartY + 2.0f), 'D', '3', 'D', '1', '2', ' ', 'D', 'E', 'V', 'I', 1.3f)) panelCol = float3(0.8f, 0.85f, 0.9f);

                float line1Y = logStartY + 20.0f;
                if (px.x >= 20.0f && px.x <= 60.0f && px.y >= line1Y && px.y <= line1Y + 14.0f) panelCol = float3(0.15f, 0.35f, 0.65f);
                if (Str4(px, float2(24.0f, line1Y + 2.0f), 'I', 'N', 'F', 'O', 1.3f)) panelCol = float3(1, 1, 1);
                if (Str10(px, float2(70.0f, line1Y + 2.0f), 'P', 'H', 'Y', 'S', 'I', 'C', 'S', ' ', '6', '0', 1.3f)) panelCol = float3(0.3f, 0.9f, 0.5f);

                float line2Y = logStartY + 40.0f;
                if (px.x >= 20.0f && px.x <= 60.0f && px.y >= line2Y && px.y <= line2Y + 14.0f) panelCol = float3(0.15f, 0.35f, 0.65f);
                if (Str4(px, float2(24.0f, line2Y + 2.0f), 'I', 'N', 'F', 'O', 1.3f)) panelCol = float3(1, 1, 1);
                if (Str10(px, float2(70.0f, line2Y + 2.0f), 'E', 'N', 'T', 'I', 'T', 'Y', ' ', 'S', 'P', 'A', 1.3f)) panelCol = float3(0.8f, 0.85f, 0.9f);
            }

            if (px.y >= h - 22.0f) {
                panelCol = float3(0.07f, 0.07f, 0.10f);
                if (px.x >= 15.0f && px.x <= 23.0f && px.y >= h - 15.0f && px.y <= h - 7.0f) panelCol = float3(0.2f, 0.9f, 0.4f);
                if (Str5(px, float2(32.0f, h - 15.0f), 'R', 'E', 'A', 'D', 'Y', 1.4f)) panelCol = float3(0.7f, 0.9f, 0.7f);
                if (Str8(px, float2(100.0f, h - 15.0f), 'S', 'H', 'I', 'P', 'P', 'I', 'N', 'G', 1.4f)) panelCol = float3(0.3f, 0.85f, 1.0f);
                if (Str6(px, float2(200.0f, h - 15.0f), '6', '0', ' ', 'F', 'P', 'S', 1.4f)) panelCol = float3(0.6f, 0.8f, 1.0f);
            }
        }

        if (px.y <= h - bottomDockH + 1.0f) panelCol = float3(0.20f, 0.22f, 0.30f);
        finalColor = panelCol;
    }

    // =========================================================================
    // 6. POPUPS & MODALS: (Param6 == 2: Build Success | Param6 == 3: Add Component)
    // =========================================================================
    if (Param6 > 1.5f) {
        float2 modCenter = float2(w * 0.5f, h * 0.5f);
        float2 modSize = (Param6 > 2.5f) ? float2(440.0f, 320.0f) : float2(540.0f, 260.0f);
        float2 modMin = modCenter - modSize * 0.5f;
        float2 modMax = modCenter + modSize * 0.5f;

        finalColor = lerp(finalColor, float3(0.02f, 0.03f, 0.05f), 0.75f);

        if (px.x >= modMin.x && px.x <= modMax.x && px.y >= modMin.y && px.y <= modMax.y) {
            finalColor = float3(0.12f, 0.14f, 0.19f);

            // Modal Header
            if (px.y <= modMin.y + 34.0f) {
                finalColor = float3(0.16f, 0.24f, 0.35f);
                if (Param6 > 2.5f) {
                    if (Str10(px, float2(modMin.x + 20.0f, modMin.y + 10.0f), 'A', 'D', 'D', ' ', 'C', 'O', 'M', 'P', 'O', 'N', 1.5f)) finalColor = float3(1, 1, 1);
                } else {
                    if (Str10(px, float2(modMin.x + 20.0f, modMin.y + 10.0f), 'B', 'U', 'I', 'L', 'D', ' ', 'S', 'U', 'C', 'C', 1.6f)) finalColor = float3(1, 1, 1);
                }
            }

            if (Param6 > 2.5f) { // Add Component Items
                for (int comp = 0; comp < 5; ++comp) {
                    float itemY = modMin.y + 50.0f + comp * 42.0f;
                    if (px.y >= itemY && px.y <= itemY + 36.0f && px.x >= modMin.x + 15.0f && px.x <= modMax.x - 15.0f) {
                        finalColor = float3(0.18f, 0.25f, 0.40f);
                    }
                    if (comp == 0 && Str10(px, float2(modMin.x + 30.0f, itemY + 10.0f), 'R', 'I', 'G', 'I', 'D', 'B', 'O', 'D', 'Y', ' ', 1.4f)) finalColor = float3(1, 1, 1);
                    else if (comp == 1 && Str10(px, float2(modMin.x + 30.0f, itemY + 10.0f), 'B', 'O', 'X', ' ', 'C', 'O', 'L', 'L', 'I', 'D', 1.4f)) finalColor = float3(1, 1, 1);
                    else if (comp == 2 && Str10(px, float2(modMin.x + 30.0f, itemY + 10.0f), 'A', 'U', 'D', 'I', 'O', ' ', 'S', 'O', 'U', 'R', 1.4f)) finalColor = float3(1, 1, 1);
                    else if (comp == 3 && Str10(px, float2(modMin.x + 30.0f, itemY + 10.0f), 'S', 'C', 'R', 'I', 'P', 'T', ' ', 'C', 'O', 'M', 1.4f)) finalColor = float3(1, 1, 1);
                    else if (comp == 4 && Str10(px, float2(modMin.x + 30.0f, itemY + 10.0f), 'P', 'A', 'R', 'T', 'I', 'C', 'L', 'E', ' ', 'E', 1.4f)) finalColor = float3(1, 1, 1);
                }
            } else { // Build Success Items
                if (Str10(px, float2(modMin.x + 30.0f, modMin.y + 60.0f), 'P', 'R', 'O', 'J', 'E', 'C', 'T', ' ', 'P', 'A', 1.4f)) finalColor = float3(0.3f, 0.9f, 0.5f);
                if (Str10(px, float2(modMin.x + 30.0f, modMin.y + 90.0f), 'D', 'I', 'S', 'T', ':', ' ', 'D', 'I', 'S', 'T', 1.4f)) finalColor = float3(0.8f, 0.85f, 0.95f);
                if (Str10(px, float2(modMin.x + 30.0f, modMin.y + 120.0f), 'S', 'T', 'A', 'N', 'D', 'A', 'L', 'O', 'N', 'E', 1.4f)) finalColor = float3(0.8f, 0.85f, 0.95f);

                float2 btnMin = float2(modMin.x + 30.0f, modMax.y - 60.0f);
                float2 btnMax = float2(modMin.x + 290.0f, modMax.y - 20.0f);
                if (px.x >= btnMin.x && px.x <= btnMax.x && px.y >= btnMin.y && px.y <= btnMax.y) {
                    finalColor = float3(0.18f, 0.65f, 0.35f);
                    if (Str10(px, float2(btnMin.x + 20.0f, btnMin.y + 12.0f), 'L', 'A', 'U', 'N', 'C', 'H', ' ', 'G', 'A', 'M', 1.5f)) finalColor = float3(1, 1, 1);
                }

                float2 closeMin = float2(modMax.x - 140.0f, modMax.y - 60.0f);
                float2 closeMax = float2(modMax.x - 30.0f, modMax.y - 20.0f);
                if (px.x >= closeMin.x && px.x <= closeMax.x && px.y >= closeMin.y && px.y <= closeMax.y) {
                    finalColor = float3(0.25f, 0.28f, 0.38f);
                    if (Str5(px, float2(closeMin.x + 28.0f, closeMin.y + 12.0f), 'C', 'L', 'O', 'S', 'E', 1.5f)) finalColor = float3(1, 1, 1);
                }
            }

            if (abs(px.x - modMin.x) < 1.0f || abs(px.x - modMax.x) < 1.0f || abs(px.y - modMin.y) < 1.0f || abs(px.y - modMax.y) < 1.0f) {
                finalColor = float3(0.25f, 0.70f, 0.40f);
            }
        }
    }

    // =========================================================================
    // 7. POPUP MENUS (FILE / EDIT / SCENE / BUILD)
    // =========================================================================
    int activeMenu = int(Param4);
    if (activeMenu == 1) { // FILE
        float2 dropMin = float2(80.0f, 32.0f);
        float2 dropMax = float2(220.0f, 32.0f + 115.0f);
        if (px.x >= dropMin.x && px.x <= dropMax.x && px.y >= dropMin.y && px.y <= dropMax.y) {
            finalColor = float3(0.14f, 0.15f, 0.20f);
            float inDropY = px.y - dropMin.y;
            int itemIdx = int(inDropY / 28.0f);
            if (frac(inDropY / 28.0f) < 0.95f && itemIdx < 4) finalColor = float3(0.20f, 0.23f, 0.32f);

            if (itemIdx == 0 && Str8(px, float2(dropMin.x + 14.0f, dropMin.y + 7.0f), 'N', 'E', 'W', ' ', 'S', 'C', 'E', 'N', 1.4f)) finalColor = float3(1, 1, 1);
            if (itemIdx == 1 && Str8(px, float2(dropMin.x + 14.0f, dropMin.y + 35.0f), 'O', 'P', 'E', 'N', ' ', 'S', 'C', 'E', 1.4f)) finalColor = float3(1, 1, 1);
            if (itemIdx == 2 && Str8(px, float2(dropMin.x + 14.0f, dropMin.y + 63.0f), 'S', 'A', 'V', 'E', ' ', 'S', 'C', 'E', 1.4f)) finalColor = float3(1, 1, 1);
            if (itemIdx == 3 && Str4(px, float2(dropMin.x + 14.0f, dropMin.y + 91.0f), 'E', 'X', 'I', 'T', 1.4f)) finalColor = float3(1.0f, 0.4f, 0.4f);
        }
    } else if (activeMenu == 4) { // BUILD
        float2 dropMin = float2(240.0f, 32.0f);
        float2 dropMax = float2(420.0f, 32.0f + 88.0f);
        if (px.x >= dropMin.x && px.x <= dropMax.x && px.y >= dropMin.y && px.y <= dropMax.y) {
            finalColor = float3(0.14f, 0.15f, 0.20f);
            float inDropY = px.y - dropMin.y;
            int itemIdx = int(inDropY / 28.0f);
            if (frac(inDropY / 28.0f) < 0.95f && itemIdx < 3) finalColor = float3(0.20f, 0.23f, 0.32f);

            if (itemIdx == 0 && Str10(px, float2(dropMin.x + 14.0f, dropMin.y + 7.0f), 'P', 'A', 'C', 'K', 'A', 'G', 'E', ' ', 'W', 'I', 1.4f)) finalColor = float3(0.3f, 0.9f, 0.5f);
            if (itemIdx == 1 && Str10(px, float2(dropMin.x + 14.0f, dropMin.y + 35.0f), 'B', 'U', 'I', 'L', 'D', ' ', 'A', 'L', 'L', ' ', 1.4f)) finalColor = float3(1, 1, 1);
            if (itemIdx == 2 && Str10(px, float2(dropMin.x + 14.0f, dropMin.y + 63.0f), 'C', 'L', 'E', 'A', 'N', ' ', 'B', 'U', 'I', 'L', 1.4f)) finalColor = float3(0.9f, 0.6f, 0.5f);
        }
    }

    return float4(finalColor, 1.0f);
}
