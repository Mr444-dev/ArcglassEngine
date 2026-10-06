// Arcglass Engine editor viewport preview (Direct3D 12, shader model 5.1).
// Direct3D 12 SM 5.1 / 6.0 Compatible
#include "EditorFont.hlsli"

cbuffer PushConstants : register(b0) {
    float ScreenWidth;
    float ScreenHeight;
    float CamYaw;
    float CamPitch;
    float CamX;
    float CamY;
    float CamZ;
    float PackedState;

    // 8 Active Scene Entities: type 0=Cube, 1=Sphere, 2=Cylinder, 3=Plane, 4=Directional Light marker.
    float4 Entity0;
    float4 Entity1;
    float4 Entity2;
    float4 Entity3;
    float4 Entity4;
    float4 Entity5;
    float4 Entity6;
    float4 Entity7;
    // Two values per entity: world Y rotation and uniform scale.
    float4 EntityTransform0;
    float4 EntityTransform1;
    float4 EntityTransform2;
    float4 EntityTransform3;
    float2 MousePosition;
    float SelectedPositionX;
    float SelectedPositionY;
    float SelectedPositionZ;
    float SelectedRotationDegrees;
    float SelectedScale;
    float SceneDirty;
};

// Keep small editor enums in one exact 19-bit integer instead of spending a
// separate root constant on each UI state value.
#define Param0 CamYaw
#define Param1 CamPitch
#define Param2 ((float)((uint)PackedState & 0x3u))
#define Param3 ((float)(int(((uint)PackedState >> 2u) & 0xFu) - 1))
#define Param4 ((float)(((uint)PackedState >> 6u) & 0x7u))
#define Param5 ((float)(((uint)PackedState >> 9u) & 0x1u))
#define Param6 ((float)(((uint)PackedState >> 10u) & 0x3u))
#define Param7 ((float)(((uint)PackedState >> 12u) & 0xFu))
#define Param8 CamX
#define Param9 CamY
#define Param10 CamZ
#define Param11 ((float)(((uint)PackedState >> 16u) & 0x3u))
#define Param12 ((float)(((uint)PackedState >> 18u) & 0x1u))
#define EntityComponents (((uint)PackedState >> 19u) & 0x1Fu)

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

bool IsChar(float2 px, float2 pos, uint c, float scale) {
    float2 rel = (px - pos) / max(scale, 1e-6f);
    bool inBounds = scale > 0.0f && rel.x >= 0.0f && rel.x < 8.0f && rel.y >= 0.0f && rel.y < 12.0f;
    float2 safeRel = clamp(rel, float2(0.0f, 0.0f), float2(7.999f, 11.999f));
    uint x = 7u - uint(safeRel.x);
    uint y = uint(safeRel.y);
    uint glyphIndex = min(c & 127u, 127u);
    uint glyph = FontGlyphs[glyphIndex * 3u + y / 4u];
    uint bitIndex = (y & 3u) * 8u + x;
    return inBounds && ((glyph >> bitIndex) & 1u) != 0u;
}

bool Str3(float2 px, float2 pos, uint c0, uint c1, uint c2, float s = 1.0f) {
    float w = 9.0f * s;
    return IsChar(px, pos, c0, s) || IsChar(px, pos + float2(w, 0), c1, s) || IsChar(px, pos + float2(2*w, 0), c2, s);
}

bool Str4(float2 px, float2 pos, uint c0, uint c1, uint c2, uint c3, float s = 1.0f) {
    float w = 9.0f * s;
    return IsChar(px, pos, c0, s) || IsChar(px, pos + float2(w, 0), c1, s) || IsChar(px, pos + float2(2*w, 0), c2, s) || IsChar(px, pos + float2(3*w, 0), c3, s);
}

bool Str5(float2 px, float2 pos, uint c0, uint c1, uint c2, uint c3, uint c4, float s = 1.0f) {
    float w = 9.0f * s;
    return IsChar(px, pos, c0, s) || IsChar(px, pos + float2(w, 0), c1, s) || IsChar(px, pos + float2(2*w, 0), c2, s) || IsChar(px, pos + float2(3*w, 0), c3, s) || IsChar(px, pos + float2(4*w, 0), c4, s);
}

bool Str6(float2 px, float2 pos, uint c0, uint c1, uint c2, uint c3, uint c4, uint c5, float s = 1.0f) {
    float w = 9.0f * s;
    return Str3(px, pos, c0, c1, c2, s) || Str3(px, pos + float2(3*w, 0), c3, c4, c5, s);
}

bool Str7(float2 px, float2 pos, uint c0, uint c1, uint c2, uint c3, uint c4, uint c5, uint c6, float s = 1.0f) {
    float w = 9.0f * s;
    return Str4(px, pos, c0, c1, c2, c3, s) || Str3(px, pos + float2(4*w, 0), c4, c5, c6, s);
}

bool Str8(float2 px, float2 pos, uint c0, uint c1, uint c2, uint c3, uint c4, uint c5, uint c6, uint c7, float s = 1.0f) {
    float w = 9.0f * s;
    return Str4(px, pos, c0, c1, c2, c3, s) || Str4(px, pos + float2(4*w, 0), c4, c5, c6, c7, s);
}

bool Str9(float2 px, float2 pos, uint c0, uint c1, uint c2, uint c3, uint c4, uint c5, uint c6, uint c7, uint c8, float s = 1.0f) {
    float w = 9.0f * s;
    return Str5(px, pos, c0, c1, c2, c3, c4, s) || Str4(px, pos + float2(5*w, 0), c5, c6, c7, c8, s);
}

bool Str10(float2 px, float2 pos, uint c0, uint c1, uint c2, uint c3, uint c4, uint c5, uint c6, uint c7, uint c8, uint c9, float s = 1.0f) {
    float w = 9.0f * s;
    return Str5(px, pos, c0, c1, c2, c3, c4, s) || Str5(px, pos + float2(5*w, 0), c5, c6, c7, c8, c9, s);
}

// 3D Geometric Primitive Ray Intersection Routines

bool RayBoxIntersect(float3 ro, float3 rd, float3 boxMin, float3 boxMax, inout float tNear, inout float3 normal) {
    tNear = 1e9;
    normal = float3(0.0f, 1.0f, 0.0f);
    bool hit = false;
    float hitT = 1e9;
    float3 hitNormal = float3(0.0f, 1.0f, 0.0f);

    // Avoid 0 * infinity producing NaNs for axis-aligned rays.
    float3 safeRd = float3(
        abs(rd.x) < 1e-6f ? (rd.x < 0.0f ? -1e-6f : 1e-6f) : rd.x,
        abs(rd.y) < 1e-6f ? (rd.y < 0.0f ? -1e-6f : 1e-6f) : rd.y,
        abs(rd.z) < 1e-6f ? (rd.z < 0.0f ? -1e-6f : 1e-6f) : rd.z);
    float3 invD = 1.0f / safeRd;
    float3 t0 = (boxMin - ro) * invD;
    float3 t1 = (boxMax - ro) * invD;
    float3 tMin = min(t0, t1);
    float3 tMax = max(t0, t1);

    float tEnter = max(max(tMin.x, tMin.y), tMin.z);
    float tExit  = min(min(tMax.x, tMax.y), tMax.z);

    if (tEnter <= tExit && tExit > 0.0f) {
        hitT = tEnter > 0.0f ? tEnter : tExit;
        float3 hitPoint = ro + rd * hitT;
        float3 center = (boxMin + boxMax) * 0.5f;
        float3 size = (boxMax - boxMin) * 0.5f;
        float3 d = (hitPoint - center) / size;
        float3 absD = abs(d);
        if (absD.x > absD.y && absD.x > absD.z) hitNormal = float3(sign(d.x), 0.0f, 0.0f);
        else if (absD.y > absD.z) hitNormal = float3(0.0f, sign(d.y), 0.0f);
        else hitNormal = float3(0.0f, 0.0f, sign(d.z));
        hit = true;
    }

    if (hit) {
        tNear = hitT;
        normal = hitNormal;
    }
    return hit;
}

float2 GetEntityTransform(int entityIndex) {
    float4 packed = entityIndex < 2 ? EntityTransform0 :
                    entityIndex < 4 ? EntityTransform1 :
                    entityIndex < 6 ? EntityTransform2 : EntityTransform3;
    return (entityIndex & 1) == 0 ? packed.xy : packed.zw;
}

float2 GetEntityTransform(float4 packedTransform, int entityIndex) {
    return (entityIndex & 1) == 0 ? packedTransform.xy : packedTransform.zw;
}

bool DrawFixedNumber(float2 px, float2 pos, float value) {
    float clampedValue = min(abs(value), 99.99f);
    uint hundredths = (uint)floor(clampedValue * 100.0f + 0.5f);
    uint glyph = 0u;
    bool hit = false;
    [unroll]
    for (uint index = 0u; index < 6u; ++index) {
        if (index == 0u) glyph = value < -0.005f ? (uint)'-' : (uint)'+';
        else if (index == 1u) glyph = (uint)'0' + (hundredths / 1000u) % 10u;
        else if (index == 2u) glyph = (uint)'0' + (hundredths / 100u) % 10u;
        else if (index == 3u) glyph = (uint)'.';
        else if (index == 4u) glyph = (uint)'0' + (hundredths / 10u) % 10u;
        else glyph = (uint)'0' + hundredths % 10u;
        hit = hit || IsChar(px, pos + float2((float)index * 9.0f, 0.0f), glyph, 1.0f);
    }
    return hit;
}

bool IsHovered(float2 px, float2 minimum, float2 maximum) {
    return MousePosition.x >= minimum.x && MousePosition.x <= maximum.x &&
           MousePosition.y >= minimum.y && MousePosition.y <= maximum.y &&
           px.x >= minimum.x && px.x <= maximum.x && px.y >= minimum.y && px.y <= maximum.y;
}

float SegmentDistance(float2 samplePos, float2 start, float2 segmentEnd) {
    float2 segment = segmentEnd - start;
    float amount = saturate(dot(samplePos - start, segment) / max(dot(segment, segment), 1e-5f));
    return length(samplePos - (start + segment * amount));
}

bool IconLine(float2 px, float2 start, float2 end, float thickness = 0.8f) {
    return SegmentDistance(px, start, end) <= thickness;
}

bool IconCircle(float2 px, float2 center, float radius, float thickness = 0.8f) {
    return abs(length(px - center) - radius) <= thickness;
}

bool DrawCubeIcon(float2 px, float2 origin) {
    float2 a = origin + float2(3, 6), b = origin + float2(8, 3), c = origin + float2(13, 6);
    float2 d = origin + float2(3, 12), e = origin + float2(8, 15), f = origin + float2(13, 12);
    return IconLine(px, a, b) || IconLine(px, b, c) || IconLine(px, a, d) || IconLine(px, b, origin + float2(8, 9)) ||
           IconLine(px, c, f) || IconLine(px, d, e) || IconLine(px, e, f) || IconLine(px, origin + float2(8, 9), e) ||
           IconLine(px, a, origin + float2(8, 9)) || IconLine(px, c, origin + float2(8, 9));
}

bool DrawSphereIcon(float2 px, float2 origin) {
    float2 p = px - (origin + float2(8, 9));
    float outer = abs(length(p) - 5.0f);
    float meridian = abs(length(float2(p.x * 1.9f, p.y)) - 5.0f);
    float equator = abs(length(float2(p.x, p.y * 2.1f)) - 5.0f);
    return outer <= 0.8f || (meridian <= 0.8f && abs(p.x) <= 2.8f) || (equator <= 0.8f && abs(p.y) <= 2.8f);
}

bool DrawCylinderIcon(float2 px, float2 origin) {
    float2 p = px - origin;
    float top = abs((p.x - 8.0f) * (p.x - 8.0f) / 25.0f + (p.y - 5.0f) * (p.y - 5.0f) / 4.0f - 1.0f);
    float bottom = abs((p.x - 8.0f) * (p.x - 8.0f) / 25.0f + (p.y - 12.0f) * (p.y - 12.0f) / 4.0f - 1.0f);
    return top <= 0.24f || bottom <= 0.24f || IconLine(px, origin + float2(3, 5), origin + float2(3, 12)) ||
           IconLine(px, origin + float2(13, 5), origin + float2(13, 12));
}

bool DrawPlaneIcon(float2 px, float2 origin) {
    return IconLine(px, origin + float2(2, 4), origin + float2(14, 4)) || IconLine(px, origin + float2(2, 14), origin + float2(14, 14)) ||
           IconLine(px, origin + float2(2, 4), origin + float2(2, 14)) || IconLine(px, origin + float2(14, 4), origin + float2(14, 14)) ||
           IconLine(px, origin + float2(7, 4), origin + float2(7, 14)) || IconLine(px, origin + float2(2, 9), origin + float2(14, 9));
}

bool DrawLightIcon(float2 px, float2 origin) {
    float2 center = origin + float2(8, 9);
    return IconCircle(px, center, 3.0f) || IconLine(px, center + float2(-6, 0), center + float2(-4, 0)) ||
           IconLine(px, center + float2(4, 0), center + float2(6, 0)) || IconLine(px, center + float2(0, -6), center + float2(0, -4)) ||
           IconLine(px, center + float2(0, 4), center + float2(0, 6)) || IconLine(px, center + float2(-4, -4), center + float2(-3, -3)) ||
           IconLine(px, center + float2(3, 3), center + float2(4, 4)) || IconLine(px, center + float2(3, -3), center + float2(4, -4)) ||
           IconLine(px, center + float2(-4, 4), center + float2(-3, 3));
}

bool DrawTrashIcon(float2 px, float2 origin) {
    return IconLine(px, origin + float2(4, 6), origin + float2(12, 6)) || IconLine(px, origin + float2(6, 4), origin + float2(10, 4)) ||
           IconLine(px, origin + float2(5, 7), origin + float2(6, 14)) || IconLine(px, origin + float2(11, 7), origin + float2(10, 14)) ||
           IconLine(px, origin + float2(6, 14), origin + float2(10, 14)) || IconLine(px, origin + float2(8, 8), origin + float2(8, 13));
}

bool DrawMoveIcon(float2 px, float2 origin) {
    float2 center = origin + float2(8, 9);
    return IconLine(px, center + float2(-5, 0), center + float2(5, 0)) || IconLine(px, center + float2(0, -5), center + float2(0, 5)) ||
           IconLine(px, center + float2(-5, 0), center + float2(-3, -2)) || IconLine(px, center + float2(-5, 0), center + float2(-3, 2)) ||
           IconLine(px, center + float2(5, 0), center + float2(3, -2)) || IconLine(px, center + float2(5, 0), center + float2(3, 2)) ||
           IconLine(px, center + float2(0, -5), center + float2(-2, -3)) || IconLine(px, center + float2(0, -5), center + float2(2, -3)) ||
           IconLine(px, center + float2(0, 5), center + float2(-2, 3)) || IconLine(px, center + float2(0, 5), center + float2(2, 3));
}

bool DrawRotateIcon(float2 px, float2 origin) {
    float2 center = origin + float2(8, 9);
    float2 p = px - center;
    float r = length(p);
    return abs(r - 4.5f) <= 0.8f && (p.y <= 2.5f || p.x <= -2.5f) ||
           IconLine(px, center + float2(2, -5), center + float2(5, -5)) || IconLine(px, center + float2(5, -5), center + float2(5, -2));
}

bool DrawScaleIcon(float2 px, float2 origin) {
    return IconLine(px, origin + float2(3, 14), origin + float2(13, 4)) ||
           IconLine(px, origin + float2(3, 14), origin + float2(3, 10)) || IconLine(px, origin + float2(3, 14), origin + float2(7, 14)) ||
           IconLine(px, origin + float2(13, 4), origin + float2(9, 4)) || IconLine(px, origin + float2(13, 4), origin + float2(13, 8));
}

bool DrawPlayIcon(float2 px, float2 origin) {
    return IconLine(px, origin + float2(5, 3), origin + float2(5, 15), 1.0f) ||
           IconLine(px, origin + float2(5, 3), origin + float2(14, 9), 1.0f) || IconLine(px, origin + float2(14, 9), origin + float2(5, 15), 1.0f);
}

bool DrawSelectIcon(float2 px, float2 origin) {
    float2 tip = origin + float2(4, 3);
    float2 shoulder = origin + float2(4, 15);
    float2 inner = origin + float2(8, 11);
    float2 right = origin + float2(12, 16);
    return IconLine(px, tip, shoulder) || IconLine(px, tip, origin + float2(14, 10)) ||
           IconLine(px, shoulder, inner) || IconLine(px, inner, right) || IconLine(px, right, origin + float2(14, 13)) ||
           IconLine(px, origin + float2(14, 13), inner);
}

bool RaySphereIntersect(float3 ro, float3 rd, float3 center, float radius, inout float tNear, inout float3 normal) {
    tNear = 1e9;
    normal = float3(0.0f, 1.0f, 0.0f);
    bool hit = false;

    float3 oc = ro - center;
    float b = dot(oc, rd);
    float c = dot(oc, oc) - radius * radius;
    float h = b * b - c;
    if (h >= 0.0f) {
        h = sqrt(h);
        float candidateT = -b - h;
        if (candidateT < 0.0f) candidateT = -b + h;
        if (candidateT >= 0.0f) {
            tNear = candidateT;
            normal = normalize(ro + rd * candidateT - center);
            hit = true;
        }
    }
    return hit;
}

bool RayCylinderIntersect(float3 ro, float3 rd, float3 center, float radius, float halfH, inout float tNear, inout float3 normal) {
    tNear = 1e9;
    normal = float3(0.0f, 1.0f, 0.0f);
    bool hit = false;

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
                hit = true;
            }
        }
    }
    return hit;
}

bool RayPlaneIntersect(float3 ro, float3 rd, float3 center, float2 halfSize, inout float tNear, inout float3 normal) {
    tNear = 1e9;
    normal = float3(0.0f, 1.0f, 0.0f);
    bool hit = false;

    if (abs(rd.y) >= 0.0001f) {
        float t = (center.y - ro.y) / rd.y;
        if (t > 0.0f) {
            float3 hitPoint = ro + rd * t;
            if (abs(hitPoint.x - center.x) <= halfSize.x && abs(hitPoint.z - center.z) <= halfSize.y) {
                tNear = t;
                normal = float3(0.0f, 1.0f, 0.0f);
                hit = true;
            }
        }
    }
    return hit;
}

#include "../common/ArcglassAtmosphere.hlsli"

float3 ArcEditorSunDirection() {
    float3 fallbackDirection = normalize(float3(-0.48f, 0.72f, -0.50f));
    int count = min(int(Param7), 8);
    for (int i = 0; i < count; ++i) {
        float4 entity = (i == 0) ? Entity0 : (i == 1) ? Entity1 : (i == 2) ? Entity2 : (i == 3) ? Entity3 : (i == 4) ? Entity4 : (i == 5) ? Entity5 : (i == 6) ? Entity6 : Entity7;
        if (int(entity.w) == 4 && dot(entity.xyz, entity.xyz) > 0.01f) return normalize(entity.xyz);
    }
    return fallbackDirection;
}

bool ArcglassMark(float2 px, float2 pos) {
    return IconLine(px, pos + float2(1.0f, 15.0f), pos + float2(8.0f, 1.0f), 1.8f)
        || IconLine(px, pos + float2(8.0f, 1.0f), pos + float2(15.0f, 15.0f), 1.8f)
        || IconLine(px, pos + float2(4.0f, 10.0f), pos + float2(12.0f, 10.0f), 1.5f);
}

bool ArcEditorEntityOccludes(float3 origin, float3 lightDirection, int sourceEntity, int entityCount) {
    for (int i = 0; i < entityCount; ++i) {
        if (i == sourceEntity) continue;
        float4 entity = (i == 0) ? Entity0 : (i == 1) ? Entity1 : (i == 2) ? Entity2 : (i == 3) ? Entity3 : (i == 4) ? Entity4 : (i == 5) ? Entity5 : (i == 6) ? Entity6 : Entity7;
        int type = int(entity.w);
        if (type < 0 || type > 2) continue;
        float scale = max(GetEntityTransform(i).y, 0.05f);
        float radius = (type == 1 ? 0.62f : type == 2 ? 0.74f : 0.92f) * scale;
        float tShadow = 1e9f;
        float3 ignoredNormal = float3(0.0f, 1.0f, 0.0f);
        if (RaySphereIntersect(origin, lightDirection, entity.xyz, radius, tShadow, ignoredNormal) && tShadow > 0.02f && tShadow < 80.0f) return true;
    }
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

        int numEntities = min(int(Param7), 8);
        float3 lightDir = ArcEditorSunDirection();
        float3 skyCol = ArcSkyRadiance(ro, rd, lightDir, 0.0f);
        float3 sceneCol = skyCol;

        // Analytic height-field preview with a flat object staging area and a soft grid fade.
        if (rd.y < -0.02f && ro.y > 0.0f) {
            float tGround = -ro.y / rd.y;
            [unroll] for (int refine = 0; refine < 4; ++refine) {
                float3 guess = ro + rd * tGround;
                tGround = (ArcTerrainHeight(guess.xz) - ro.y) / rd.y;
            }
            if (tGround > 0.0f && tGround < 70.0f) {
                float3 hitGround = ro + rd * tGround;
                float terrainHeight = ArcTerrainHeight(hitGround.xz);
                float3 terrainNormal = ArcTerrainNormal(hitGround.xz);
                float radius = length(hitGround.xz);
                float slope = 1.0f - saturate(terrainNormal.y);
                float rockMask = saturate(slope * 3.2f + smoothstep(0.25f, 1.8f, terrainHeight) * 0.32f);
                float variation = ArcValueNoise(hitGround.xz * 0.36f);
                float3 grassColor = lerp(float3(0.055f, 0.105f, 0.090f), float3(0.105f, 0.17f, 0.115f), variation);
                float3 rockColor = lerp(float3(0.19f, 0.205f, 0.22f), float3(0.29f, 0.30f, 0.31f), variation);
                float3 terrainColor = lerp(grassColor, rockColor, rockMask);
                float directLight = saturate(dot(terrainNormal, lightDir));
                float terrainShadow = ArcTerrainShadow(hitGround, lightDir);
                terrainColor *= 0.30f + directLight * 0.70f * terrainShadow;

                float2 gridUv = hitGround.xz;
                float2 gridLines = abs(frac(gridUv - 0.5f) - 0.5f) / max(fwidth(gridUv), 0.001f);
                float gridIntensity = 1.0f - min(min(gridLines.x, gridLines.y), 1.0f);
                float axisX = 1.0f - min(abs(hitGround.z) / max(fwidth(hitGround.z), 0.001f), 1.0f);
                float axisZ = 1.0f - min(abs(hitGround.x) / max(fwidth(hitGround.x), 0.001f), 1.0f);
                float gridWeight = 1.0f - smoothstep(5.5f, 9.0f, radius);
                terrainColor = lerp(terrainColor, float3(0.19f, 0.23f, 0.30f), gridIntensity * gridWeight * 0.68f);
                terrainColor = lerp(terrainColor, float3(0.82f, 0.20f, 0.17f), axisX * gridWeight * 0.88f);
                terrainColor = lerp(terrainColor, float3(0.18f, 0.43f, 0.82f), axisZ * gridWeight * 0.88f);

                float fogFactor = clamp(tGround / 68.0f, 0.0f, 1.0f);
                sceneCol = lerp(terrainColor, skyCol, fogFactor);
            }
        }

        // Raycast against ALL active scene entities
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
            float2 transform = GetEntityTransform(i);
            float yawCos = cos(transform.x);
            float yawSin = sin(transform.x);
            float scale = max(transform.y, 0.05f);
            float3 relOrigin = ro - pos;
            float3 localRo = float3(yawCos * relOrigin.x - yawSin * relOrigin.z,
                                    relOrigin.y,
                                    yawSin * relOrigin.x + yawCos * relOrigin.z);
            float3 localRd = float3(yawCos * rd.x - yawSin * rd.z,
                                    rd.y,
                                    yawSin * rd.x + yawCos * rd.z);
            float tHit = 1e9;
            float3 nHit = float3(0, 1, 0);

            if (type == 0) { // Cube
                float extent = 0.55f * scale;
                if (RayBoxIntersect(localRo, localRd, -extent, extent, tHit, nHit)) {
                    if (tHit < tClosest) { tClosest = tHit; hitNormal = float3(yawCos * nHit.x + yawSin * nHit.z, nHit.y, -yawSin * nHit.x + yawCos * nHit.z); hitEntity = i; hitCenter = pos; hitType = 0; }
                }
            } else if (type == 1) { // Sphere
                if (RaySphereIntersect(localRo, localRd, 0.0f, 0.60f * scale, tHit, nHit)) {
                    if (tHit < tClosest) { tClosest = tHit; hitNormal = float3(yawCos * nHit.x + yawSin * nHit.z, nHit.y, -yawSin * nHit.x + yawCos * nHit.z); hitEntity = i; hitCenter = pos; hitType = 1; }
                }
            } else if (type == 2) { // Cylinder
                if (RayCylinderIntersect(localRo, localRd, 0.0f, 0.50f * scale, 0.70f * scale, tHit, nHit)) {
                    if (tHit < tClosest) { tClosest = tHit; hitNormal = float3(yawCos * nHit.x + yawSin * nHit.z, nHit.y, -yawSin * nHit.x + yawCos * nHit.z); hitEntity = i; hitCenter = pos; hitType = 2; }
                }
            } else if (type == 3) { // Plane
                if (RayPlaneIntersect(localRo, localRd, 0.0f, float2(0.5f * scale, 0.5f * scale), tHit, nHit)) {
                    if (tHit < tClosest) { tClosest = tHit; hitNormal = nHit; hitEntity = i; hitCenter = pos; hitType = 3; }
                }
            } else if (type == 4) { // Light Marker
                float extent = 0.25f * scale;
                if (RayBoxIntersect(localRo, localRd, -extent, extent, tHit, nHit)) {
                    if (tHit < tClosest) { tClosest = tHit; hitNormal = float3(yawCos * nHit.x + yawSin * nHit.z, nHit.y, -yawSin * nHit.x + yawCos * nHit.z); hitEntity = i; hitCenter = pos; hitType = 4; }
                }
            }
        }

        // If entity hit, shade with PBR lighting
        if (hitEntity >= 0) {
            float3 hitPosition = ro + rd * tClosest;
            float3 viewDir = -rd;
            bool inShadow = ArcEditorEntityOccludes(hitPosition + hitNormal * 0.025f, lightDir, hitEntity, numEntities);
            float NdotL = inShadow ? 0.0f : max(dot(hitNormal, lightDir), 0.0f);
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

            float3 ambient = albedo * (0.15f + 0.10f * saturate(hitNormal.y));
            float3 diffuse = albedo * NdotL * (inShadow ? 0.04f : 1.15f);
            float3 specular = float3(1, 1, 1) * spec * (inShadow ? 0.02f : 0.45f);
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
                sceneCol = gMode == 0 ? float3(0.20f, 0.45f, 0.85f) : IsHovered(px, vpMin + float2(8.0f, 3.0f), vpMin + float2(32.0f, 23.0f)) ? float3(0.24f, 0.27f, 0.36f) : float3(0.16f, 0.18f, 0.24f);
            }
            if (DrawSelectIcon(px, vpMin + float2(12.0f, 4.0f))) sceneCol = float3(0.94f, 0.96f, 1.0f);

            if (px.x >= vpMin.x + 36.0f && px.x <= vpMin.x + 60.0f && px.y >= vpMin.y + 3.0f && px.y <= vpMin.y + 23.0f) {
                sceneCol = gMode == 1 ? float3(0.20f, 0.45f, 0.85f) : IsHovered(px, vpMin + float2(36.0f, 3.0f), vpMin + float2(60.0f, 23.0f)) ? float3(0.24f, 0.27f, 0.36f) : float3(0.16f, 0.18f, 0.24f);
            }
            if (DrawMoveIcon(px, vpMin + float2(40.0f, 4.0f))) sceneCol = float3(0.94f, 0.96f, 1.0f);

            if (px.x >= vpMin.x + 64.0f && px.x <= vpMin.x + 88.0f && px.y >= vpMin.y + 3.0f && px.y <= vpMin.y + 23.0f) {
                sceneCol = gMode == 2 ? float3(0.20f, 0.45f, 0.85f) : IsHovered(px, vpMin + float2(64.0f, 3.0f), vpMin + float2(88.0f, 23.0f)) ? float3(0.24f, 0.27f, 0.36f) : float3(0.16f, 0.18f, 0.24f);
            }
            if (DrawRotateIcon(px, vpMin + float2(68.0f, 4.0f))) sceneCol = float3(0.94f, 0.96f, 1.0f);

            if (px.x >= vpMin.x + 92.0f && px.x <= vpMin.x + 116.0f && px.y >= vpMin.y + 3.0f && px.y <= vpMin.y + 23.0f) {
                sceneCol = gMode == 3 ? float3(0.20f, 0.45f, 0.85f) : IsHovered(px, vpMin + float2(92.0f, 3.0f), vpMin + float2(116.0f, 23.0f)) ? float3(0.24f, 0.27f, 0.36f) : float3(0.16f, 0.18f, 0.24f);
            }
            if (DrawScaleIcon(px, vpMin + float2(96.0f, 4.0f))) sceneCol = float3(0.94f, 0.96f, 1.0f);

            if (Str5(px, float2(vpMin.x + 135.0f, vpMin.y + 7.0f), 'S', 'C', 'E', 'N', 'E', 1.0f)) sceneCol = float3(0.65f, 0.7f, 0.85f);
            if (Str4(px, float2(vpMin.x + 190.0f, vpMin.y + 7.0f), 'D', 'X', '1', '2', 1.0f)) sceneCol = float3(0.4f, 0.75f, 1.0f);

            // Simulation state badge
            int simState = int(Param2);
            if (simState == 1) {
                if (px.x >= vpMax.x - 90.0f && px.x <= vpMax.x - 10.0f && px.y >= vpMin.y + 4.0f && px.y <= vpMin.y + 22.0f) sceneCol = float3(0.18f, 0.65f, 0.35f);
                if (Str7(px, float2(vpMax.x - 80.0f, vpMin.y + 7.0f), 'P', 'L', 'A', 'Y', 'I', 'N', 'G', 1.0f)) sceneCol = float3(1, 1, 1);
            } else if (simState == 2) {
                if (px.x >= vpMax.x - 85.0f && px.x <= vpMax.x - 10.0f && px.y >= vpMin.y + 4.0f && px.y <= vpMin.y + 22.0f) sceneCol = float3(0.70f, 0.55f, 0.15f);
                if (Str6(px, float2(vpMax.x - 75.0f, vpMin.y + 7.0f), 'P', 'A', 'U', 'S', 'E', 'D', 1.0f)) sceneCol = float3(1, 1, 1);
            } else {
                if (px.x >= vpMax.x - 80.0f && px.x <= vpMax.x - 10.0f && px.y >= vpMin.y + 4.0f && px.y <= vpMin.y + 22.0f) sceneCol = float3(0.18f, 0.30f, 0.50f);
                if (Str4(px, float2(vpMax.x - 65.0f, vpMin.y + 7.0f), 'E', 'D', 'I', 'T', 1.0f)) sceneCol = float3(1, 1, 1);
            }
        }

        if (abs(px.x - vpMin.x) < 1.0f || abs(px.x - vpMax.x) < 1.0f || abs(px.y - vpMin.y) < 1.0f || abs(px.y - vpMax.y) < 1.0f) {
            sceneCol = float3(0.20f, 0.22f, 0.30f);
        }
        finalColor = sceneCol;
    }

    // =========================================================================
    // 2. ARCGLASS EDITOR TOOLBAR & SIMULATION CONTROLS
    // =========================================================================
    if (px.y < topBarH) {
        float3 barCol = float3(0.10f, 0.11f, 0.15f);

        // Menu Bar Row (Y: 0 to 32)
        if (px.y < 32.0f) {
            if (px.x < 95.0f) barCol = IsHovered(px, float2(8.0f, 2.0f), float2(96.0f, 29.0f)) ? float3(0.18f, 0.24f, 0.36f) : float3(0.14f, 0.19f, 0.29f);

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

            if (ArcglassMark(px, float2(8.0f, 7.0f))) barCol = float3(0.42f, 0.78f, 0.98f);
            if (Str8(px, float2(29.0f, 7.0f), 'A', 'R', 'C', 'G', 'L', 'A', 'S', 'S', 0.78f)) barCol = float3(0.70f, 0.87f, 1.0f);
            if (Str4(px, float2(90.0f, 8.0f), 'F', 'I', 'L', 'E', 1.0f)) barCol = float3(0.95f, 0.95f, 1.0f);
            if (Str4(px, float2(140.0f, 8.0f), 'E', 'D', 'I', 'T', 1.0f)) barCol = float3(0.95f, 0.95f, 1.0f);
            if (Str5(px, float2(190.0f, 8.0f), 'S', 'C', 'E', 'N', 'E', 1.0f)) barCol = float3(0.95f, 0.95f, 1.0f);
            if (Str5(px, float2(250.0f, 8.0f), 'B', 'U', 'I', 'L', 'D', 1.0f)) barCol = float3(0.95f, 0.95f, 1.0f);

            if (DrawPlayIcon(px, float2(centerX - 103.0f, 8.0f))) barCol = float3(0.94f, 1.0f, 0.96f);
            if (IconLine(px, float2(centerX - 42.0f, 9.0f), float2(centerX - 42.0f, 23.0f), 1.5f) ||
                IconLine(px, float2(centerX - 34.0f, 9.0f), float2(centerX - 34.0f, 23.0f), 1.5f)) barCol = float3(1.0f, 0.91f, 0.65f);
            if (DrawPlayIcon(px, float2(centerX + 8.0f, 8.0f)) ||
                IconLine(px, float2(centerX + 21.0f, 12.0f), float2(centerX + 21.0f, 20.0f), 1.1f)) barCol = float3(0.82f, 0.91f, 1.0f);
            if (px.x >= centerX + 66.0f && px.x <= centerX + 80.0f && px.y >= 9.0f && px.y <= 23.0f) barCol = float3(1.0f, 0.84f, 0.84f);

            // [PACKAGE / BUILD GAME] Toolbar Button
            if (px.x >= w - 150.0f && px.x <= w - 15.0f && px.y >= 5.0f && px.y <= 27.0f) barCol = float3(0.18f, 0.45f, 0.75f);
            if (Str10(px, float2(w - 142.0f, 8.0f), 'B', 'U', 'I', 'L', 'D', ' ', 'G', 'A', 'M', 'E', 1.0f)) barCol = float3(1, 1, 1);
        }
        // Quick Object Creation Row (Y: 33 to 74)
        else {
            barCol = float3(0.12f, 0.13f, 0.18f);

            if (IsHovered(px, float2(12.0f, 38.0f), float2(82.0f, 68.0f))) barCol = float3(0.22f, 0.28f, 0.42f);
            if (DrawCubeIcon(px, float2(17.0f, 43.0f))) barCol = float3(0.93f, 0.59f, 0.39f);
            if (Str4(px, float2(39.0f, 48.0f), 'C', 'U', 'B', 'E', 1.0f)) barCol = float3(0.94f, 0.95f, 1.0f);

            if (IsHovered(px, float2(88.0f, 38.0f), float2(170.0f, 68.0f))) barCol = float3(0.22f, 0.28f, 0.42f);
            if (DrawSphereIcon(px, float2(94.0f, 43.0f))) barCol = float3(0.49f, 0.75f, 1.0f);
            if (Str6(px, float2(116.0f, 48.0f), 'S', 'P', 'H', 'E', 'R', 'E', 1.0f)) barCol = float3(0.94f, 0.95f, 1.0f);

            if (IsHovered(px, float2(176.0f, 38.0f), float2(265.0f, 68.0f))) barCol = float3(0.22f, 0.28f, 0.42f);
            if (DrawCylinderIcon(px, float2(183.0f, 43.0f))) barCol = float3(0.49f, 0.83f, 0.55f);
            if (Str3(px, float2(205.0f, 48.0f), 'C', 'Y', 'L', 1.0f)) barCol = float3(0.94f, 0.95f, 1.0f);

            if (IsHovered(px, float2(271.0f, 38.0f), float2(348.0f, 68.0f))) barCol = float3(0.22f, 0.28f, 0.42f);
            if (DrawPlaneIcon(px, float2(277.0f, 43.0f))) barCol = float3(0.81f, 0.84f, 0.90f);
            if (Str5(px, float2(299.0f, 48.0f), 'P', 'L', 'A', 'N', 'E', 1.0f)) barCol = float3(0.94f, 0.95f, 1.0f);

            if (IsHovered(px, float2(354.0f, 38.0f), float2(430.0f, 68.0f))) barCol = float3(0.28f, 0.26f, 0.16f);
            if (DrawLightIcon(px, float2(360.0f, 43.0f))) barCol = float3(1.0f, 0.86f, 0.35f);
            if (Str5(px, float2(382.0f, 48.0f), 'L', 'I', 'G', 'H', 'T', 1.0f)) barCol = float3(1.0f, 0.93f, 0.59f);

            // [DEBUG DRAW F5]
            if (IsHovered(px, float2(436.0f, 38.0f), float2(525.0f, 68.0f))) {
                barCol = (Param12 > 0.5f) ? float3(0.18f, 0.55f, 0.30f) : float3(0.18f, 0.20f, 0.28f);
            }
            if (Str8(px, float2(444.0f, 48.0f), 'D', 'E', 'B', 'U', 'G', ' ', 'F', '5', 1.0f)) barCol = float3(0.84f, 0.88f, 0.95f);

            // [🗑 DELETE]
            if (IsHovered(px, float2(531.0f, 38.0f), float2(605.0f, 68.0f))) barCol = float3(0.38f, 0.16f, 0.18f);
            if (DrawTrashIcon(px, float2(539.0f, 43.0f))) barCol = float3(1.0f, 0.52f, 0.54f);
            if (Str3(px, float2(561.0f, 48.0f), 'D', 'E', 'L', 1.0f)) barCol = float3(1.0f, 0.72f, 0.72f);
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
            if (Str8(px, float2(15.0f, topBarH + 9.0f), 'O', 'U', 'T', 'L', 'I', 'N', 'E', 'R', 1.0f)) panelCol = float3(0.9f, 0.9f, 0.95f);
        } else {
            float rowY = px.y - (topBarH + 34.0f);
            uint rowIndex = uint(max(rowY, 0.0f) / 30.0f);
            float inRowY = frac(rowY / 30.0f);

            uint selId = uint(max(Param3, 0.0f));
            uint count = min(uint(max(Param7, 0.0f)), 8u);

            if (rowIndex == selId && rowIndex < count) {
                panelCol = float3(0.18f, 0.35f, 0.65f);
                if (px.x <= 4.0f) panelCol = float3(1.0f, 0.75f, 0.0f); // Yellow accent
            } else if ((rowIndex & 1u) == 1u && rowIndex < count) {
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
                if (type == 0 && Str4(px, float2(36.0f, itemY), 'C', 'U', 'B', 'E', 1.0f)) panelCol = float3(1, 1, 1);
                else if (type == 1 && Str6(px, float2(36.0f, itemY), 'S', 'P', 'H', 'E', 'R', 'E', 1.0f)) panelCol = float3(1, 1, 1);
                else if (type == 2 && Str8(px, float2(36.0f, itemY), 'C', 'Y', 'L', 'I', 'N', 'D', 'E', 'R', 1.0f)) panelCol = float3(1, 1, 1);
                else if (type == 3 && Str5(px, float2(36.0f, itemY), 'P', 'L', 'A', 'N', 'E', 1.0f)) panelCol = float3(1, 1, 1);
                else if (type == 4 && Str5(px, float2(36.0f, itemY), 'L', 'I', 'G', 'H', 'T', 1.0f)) panelCol = float3(1, 1, 1);
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
            if (Str7(px, float2(w - rightDockW + 15.0f, topBarH + 9.0f), 'D', 'E', 'T', 'A', 'I', 'L', 'S', 1.0f)) panelCol = float3(0.9f, 0.9f, 0.95f);
        } else {
            float relY = px.y - (topBarH + 36.0f);

            // SECTION 1: TRANSFORM
            if (relY >= 0.0f && relY <= 24.0f) {
                panelCol = float3(0.15f, 0.17f, 0.25f);
                if (Str9(px, float2(w - rightDockW + 15.0f, topBarH + 42.0f), 'T', 'R', 'A', 'N', 'S', 'F', 'O', 'R', 'M', 1.0f)) panelCol = float3(1, 1, 1);
            }
            if (selId < 0) {
                if (Str10(px, float2(w - rightDockW + 15.0f, topBarH + 72.0f), 'S', 'E', 'L', 'E', 'C', 'T', ' ', 'A', 'N', 'O', 1.0f)) panelCol = float3(0.60f, 0.66f, 0.76f);
                if (Str5(px, float2(w - rightDockW + 15.0f, topBarH + 88.0f), 'O', 'B', 'J', 'E', 'C', 1.0f)) panelCol = float3(0.60f, 0.66f, 0.76f);
            } else if (relY >= 30.0f && relY <= 160.0f) {
                int rowIdx = (int)floor((relY - 30.0f) / 26.0f);
                float yRow = topBarH + 68.0f + (float)rowIdx * 26.0f;
                if (rowIdx == 0 && Str5(px, float2(w - rightDockW + 15.0f, yRow), 'P', 'O', 'S', ' ', 'X', 1.0f)) panelCol = float3(0.95f, 0.48f, 0.46f);
                else if (rowIdx == 1 && Str5(px, float2(w - rightDockW + 15.0f, yRow), 'P', 'O', 'S', ' ', 'Y', 1.0f)) panelCol = float3(0.46f, 0.88f, 0.55f);
                else if (rowIdx == 2 && Str5(px, float2(w - rightDockW + 15.0f, yRow), 'P', 'O', 'S', ' ', 'Z', 1.0f)) panelCol = float3(0.49f, 0.68f, 1.0f);
                else if (rowIdx == 3 && Str5(px, float2(w - rightDockW + 15.0f, yRow), 'R', 'O', 'T', ' ', 'Y', 1.0f)) panelCol = float3(0.80f, 0.69f, 1.0f);
                else if (rowIdx == 4 && Str5(px, float2(w - rightDockW + 15.0f, yRow), 'S', 'C', 'A', 'L', 'E', 1.0f)) panelCol = float3(0.50f, 0.82f, 0.92f);

                float2 minusMin = float2(w - rightDockW + 80.0f, yRow - 2.0f);
                float2 minusMax = minusMin + float2(25.0f, 18.0f);
                if (px.x >= minusMin.x && px.x <= minusMax.x && px.y >= minusMin.y && px.y <= minusMax.y) {
                    panelCol = IsHovered(px, minusMin, minusMax) ? float3(0.33f, 0.38f, 0.50f) : float3(0.20f, 0.23f, 0.31f);
                    if (IsChar(px, float2(w - rightDockW + 90.0f, yRow), '-', 1.0f)) panelCol = float3(1, 1, 1);
                }

                float2 valueMin = float2(w - rightDockW + 112.0f, yRow - 2.0f);
                float2 valueMax = valueMin + float2(113.0f, 18.0f);
                if (px.x >= valueMin.x && px.x <= valueMax.x && px.y >= valueMin.y && px.y <= valueMax.y) {
                    panelCol = IsHovered(px, valueMin, valueMax) ? float3(0.17f, 0.20f, 0.28f) : float3(0.12f, 0.14f, 0.19f);
                    float numberValue = rowIdx == 0 ? SelectedPositionX : rowIdx == 1 ? SelectedPositionY : rowIdx == 2 ? SelectedPositionZ : rowIdx == 3 ? SelectedRotationDegrees : SelectedScale;
                    if (DrawFixedNumber(px, float2(w - rightDockW + 140.0f, yRow), numberValue)) panelCol = float3(0.91f, 0.94f, 1.0f);
                }

                float2 plusMin = float2(w - rightDockW + 232.0f, yRow - 2.0f);
                float2 plusMax = plusMin + float2(25.0f, 18.0f);
                if (px.x >= plusMin.x && px.x <= plusMax.x && px.y >= plusMin.y && px.y <= plusMax.y) {
                    panelCol = IsHovered(px, plusMin, plusMax) ? float3(0.33f, 0.38f, 0.50f) : float3(0.20f, 0.23f, 0.31f);
                    if (IsChar(px, float2(w - rightDockW + 241.0f, yRow), '+', 1.0f)) panelCol = float3(1, 1, 1);
                }
            } else if (relY >= 168.0f && relY <= 324.0f) {
                panelCol = float3(0.08f, 0.09f, 0.12f);
                float componentY = topBarH + 238.0f;
                if (relY <= 196.0f) {
                    panelCol = float3(0.15f, 0.17f, 0.25f);
                    if (Str10(px, float2(w - rightDockW + 18.0f, topBarH + 212.0f), 'C', 'O', 'M', 'P', 'O', 'N', 'E', 'N', 'T', 'S', 1.0f)) panelCol = float3(0.93f, 0.94f, 0.98f);
                }
                bool hasComponent = false;
                if ((EntityComponents & 1u) != 0u) {
                    hasComponent = true;
                    if (Str9(px, float2(w - rightDockW + 18.0f, componentY), 'R', 'I', 'G', 'I', 'D', 'B', 'O', 'D', 'Y', 1.0f)) panelCol = float3(0.82f, 0.85f, 0.92f);
                }
                if ((EntityComponents & 2u) != 0u) {
                    hasComponent = true;
                    if (Str10(px, float2(w - rightDockW + 18.0f, componentY + 26.0f), 'B', 'O', 'X', ' ', 'C', 'O', 'L', 'L', 'I', 'D', 1.0f)) panelCol = float3(0.82f, 0.85f, 0.92f);
                }
                if ((EntityComponents & 4u) != 0u) {
                    hasComponent = true;
                    if (Str10(px, float2(w - rightDockW + 18.0f, componentY + 52.0f), 'A', 'U', 'D', 'I', 'O', ' ', 'S', 'O', 'U', 'R', 1.0f)) panelCol = float3(0.82f, 0.85f, 0.92f);
                }
                if ((EntityComponents & 8u) != 0u) {
                    hasComponent = true;
                    if (Str6(px, float2(w - rightDockW + 18.0f, componentY + 78.0f), 'S', 'C', 'R', 'I', 'P', 'T', 1.0f)) panelCol = float3(0.82f, 0.85f, 0.92f);
                }
                if ((EntityComponents & 16u) != 0u) {
                    hasComponent = true;
                    if (Str10(px, float2(w - rightDockW + 18.0f, componentY + 104.0f), 'P', 'A', 'R', 'T', 'I', 'C', 'L', 'E', ' ', 'E', 1.0f)) panelCol = float3(0.82f, 0.85f, 0.92f);
                }
                if (!hasComponent && Str5(px, float2(w - rightDockW + 18.0f, componentY), 'E', 'M', 'P', 'T', 'Y', 1.0f)) panelCol = float3(0.54f, 0.58f, 0.67f);
            } else if (relY >= 340.0f && relY <= 370.0f) {
                float2 btnMin = float2(w - rightDockW + 20.0f, topBarH + 376.0f);
                float2 btnMax = float2(w - 20.0f, topBarH + 406.0f);
                panelCol = IsHovered(px, btnMin, btnMax) ? float3(0.24f, 0.43f, 0.70f) : float3(0.16f, 0.29f, 0.49f);
                if (Str10(px, float2(btnMin.x + 35.0f, btnMin.y + 9.0f), '+', ' ', 'A', 'D', 'D', ' ', 'C', 'O', 'M', 'P', 1.0f)) panelCol = float3(1, 1, 1);
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

            if (Str6(px, float2(25.0f, h - bottomDockH + 8.0f), 'C', 'R', 'E', 'A', 'T', 'E', 1.0f)) panelCol = (tab == 0) ? float3(1, 1, 1) : float3(0.6f, 0.65f, 0.75f);
            if (Str9(px, float2(170.0f, h - bottomDockH + 8.0f), 'S', 'H', 'O', 'R', 'T', 'C', 'U', 'T', 'S', 1.0f)) panelCol = (tab == 1) ? float3(1, 1, 1) : float3(0.6f, 0.65f, 0.75f);
        } else {
            if (tab == 0) {
                // Scene-creation help
                if (px.x < 180.0f) {
                    panelCol = float3(0.09f, 0.10f, 0.14f);
                    float f0Y = h - bottomDockH + 36.0f;
                    if (Str10(px, float2(15.0f, f0Y + 8.0f), 'P', 'R', 'I', 'M', 'I', 'T', 'I', 'V', 'E', 'S', 1.0f)) panelCol = float3(0.80f, 0.84f, 0.93f);
                    if (Str10(px, float2(15.0f, f0Y + 38.0f), 'A', 'D', 'D', ' ', 'O', 'B', 'J', 'E', 'C', 'T', 1.0f)) panelCol = float3(0.57f, 0.63f, 0.74f);
                    if (Str10(px, float2(15.0f, f0Y + 58.0f), 'C', 'L', 'I', 'C', 'K', ' ', 'C', 'A', 'R', 'D', 1.0f)) panelCol = float3(0.57f, 0.63f, 0.74f);
                    if (Str6(px, float2(15.0f, f0Y + 76.0f), 'T', 'O', ' ', 'A', 'D', 'D', 1.0f)) panelCol = float3(0.57f, 0.63f, 0.74f);
                    if (px.x >= 179.0f) panelCol = float3(0.20f, 0.22f, 0.30f);
                } else {
                    for (int c = 0; c < 5; ++c) {
                        float cMinX = 200.0f + c * 115.0f;
                        float cMaxX = cMinX + 100.0f;
                        float cMinY = h - bottomDockH + 38.0f;
                        float cMaxY = cMinY + 100.0f;

                        if (px.x >= cMinX && px.x <= cMaxX && px.y >= cMinY && px.y <= cMaxY) {
                            panelCol = float3(0.14f, 0.16f, 0.22f);
                            if (px.y <= cMinY + 65.0f) panelCol = float3(0.12f, 0.14f, 0.20f);
                        }
                        bool iconHit = c == 0 ? DrawCubeIcon(px, float2(cMinX + 42.0f, cMinY + 10.0f)) :
                                       c == 1 ? DrawSphereIcon(px, float2(cMinX + 42.0f, cMinY + 10.0f)) :
                                       c == 2 ? DrawCylinderIcon(px, float2(cMinX + 42.0f, cMinY + 10.0f)) :
                                       c == 3 ? DrawPlaneIcon(px, float2(cMinX + 42.0f, cMinY + 10.0f)) : DrawLightIcon(px, float2(cMinX + 42.0f, cMinY + 10.0f));
                        if (iconHit) panelCol = c == 0 ? float3(0.95f, 0.59f, 0.40f) : c == 1 ? float3(0.52f, 0.78f, 1.0f) : c == 2 ? float3(0.53f, 0.89f, 0.59f) : c == 3 ? float3(0.86f, 0.88f, 0.95f) : float3(1.0f, 0.86f, 0.36f);
                        float labelY = cMinY + 74.0f;
                        if (c == 0 && Str4(px, float2(cMinX + 18.0f, labelY), 'C', 'U', 'B', 'E', 1.0f)) panelCol = float3(1, 1, 1);
                        else if (c == 1 && Str6(px, float2(cMinX + 12.0f, labelY), 'S', 'P', 'H', 'E', 'R', 'E', 1.0f)) panelCol = float3(1, 1, 1);
                        else if (c == 2 && Str8(px, float2(cMinX + 14.0f, labelY), 'C', 'Y', 'L', 'I', 'N', 'D', 'E', 'R', 1.0f)) panelCol = float3(1, 1, 1);
                        else if (c == 3 && Str5(px, float2(cMinX + 16.0f, labelY), 'P', 'L', 'A', 'N', 'E', 1.0f)) panelCol = float3(1, 1, 1);
                        else if (c == 4 && Str5(px, float2(cMinX + 16.0f, labelY), 'L', 'I', 'G', 'H', 'T', 1.0f)) panelCol = float3(1, 1, 1);
                    }
                }
            } else {
                // Truthful keyboard reference for implemented editor actions
                panelCol = float3(0.06f, 0.07f, 0.09f);
                float logStartY = h - bottomDockH + 34.0f;
                if (Str6(px, float2(24.0f, logStartY + 10.0f), 'C', 'T', 'R', 'L', '+', 'S', 1.0f) || Str10(px, float2(120.0f, logStartY + 10.0f), 'S', 'A', 'V', 'E', ' ', 'S', 'C', 'E', 'N', 'E', 1.0f)) panelCol = float3(0.84f, 0.88f, 0.98f);
                if (Str6(px, float2(24.0f, logStartY + 36.0f), 'C', 'T', 'R', 'L', '+', 'O', 1.0f) || Str10(px, float2(120.0f, logStartY + 36.0f), 'O', 'P', 'E', 'N', ' ', 'S', 'C', 'E', 'N', 'E', 1.0f)) panelCol = float3(0.84f, 0.88f, 0.98f);
                if (Str8(px, float2(24.0f, logStartY + 62.0f), 'C', 'T', 'R', 'L', '+', 'Z', '/', 'Y', 1.0f) || Str9(px, float2(120.0f, logStartY + 62.0f), 'U', 'N', 'D', 'O', '/', 'R', 'E', 'D', 'O', 1.0f)) panelCol = float3(0.84f, 0.88f, 0.98f);
                if (Str7(px, float2(24.0f, logStartY + 88.0f), 'Q', ' ', 'W', ' ', 'E', ' ', 'R', 1.0f) || Str10(px, float2(120.0f, logStartY + 88.0f), 'G', 'I', 'Z', 'M', 'O', ' ', 'T', 'O', 'O', 'L', 1.0f)) panelCol = float3(0.84f, 0.88f, 0.98f);
                if (Str8(px, float2(24.0f, logStartY + 114.0f), 'R', 'M', 'B', '+', 'W', 'A', 'S', 'D', 1.0f) || Str6(px, float2(120.0f, logStartY + 114.0f), 'C', 'A', 'M', 'E', 'R', 'A', 1.0f)) panelCol = float3(0.84f, 0.88f, 0.98f);
            }

            if (px.y >= h - 22.0f) {
                panelCol = float3(0.07f, 0.07f, 0.10f);
                if (px.x >= 15.0f && px.x <= 23.0f && px.y >= h - 15.0f && px.y <= h - 7.0f) panelCol = float3(0.2f, 0.9f, 0.4f);
                if (Str5(px, float2(32.0f, h - 15.0f), 'D', '3', 'D', '1', '2', 1.0f)) panelCol = float3(0.55f, 0.79f, 1.0f);
                if (Str5(px, float2(100.0f, h - 15.0f), 'S', 'C', 'E', 'N', 'E', 1.0f)) panelCol = float3(0.72f, 0.76f, 0.84f);
                if (SceneDirty > 0.5f) {
                    if (Str5(px, float2(158.0f, h - 15.0f), 'D', 'I', 'R', 'T', 'Y', 1.0f)) panelCol = float3(1.0f, 0.73f, 0.34f);
                } else if (Str5(px, float2(158.0f, h - 15.0f), 'S', 'A', 'V', 'E', 'D', 1.0f)) panelCol = float3(0.51f, 0.83f, 0.64f);
            }
        }

        if (px.y <= h - bottomDockH + 1.0f) panelCol = float3(0.20f, 0.22f, 0.30f);
        finalColor = panelCol;
    }

    // =========================================================================
    // 6. POPUPS & MODALS: (Param6 == 2: Package Ready | Param6 == 3: Add Component)
    // =========================================================================
    if (Param6 > 1.0f) {
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
                    if (Str10(px, float2(modMin.x + 20.0f, modMin.y + 10.0f), 'A', 'D', 'D', ' ', 'C', 'O', 'M', 'P', 'O', 'N', 1.0f)) finalColor = float3(1, 1, 1);
                } else {
                    if (Str10(px, float2(modMin.x + 20.0f, modMin.y + 10.0f), 'B', 'U', 'I', 'L', 'D', ' ', 'S', 'U', 'C', 'C', 1.0f)) finalColor = float3(1, 1, 1);
                }
            }

            if (Param6 > 2.5f) { // Add Component Items
                for (int comp = 0; comp < 5; ++comp) {
                    float itemY = modMin.y + 50.0f + comp * 42.0f;
                    if (px.y >= itemY && px.y <= itemY + 36.0f && px.x >= modMin.x + 15.0f && px.x <= modMax.x - 15.0f) {
                        finalColor = float3(0.18f, 0.25f, 0.40f);
                    }
                    if (comp == 0 && Str10(px, float2(modMin.x + 30.0f, itemY + 10.0f), 'R', 'I', 'G', 'I', 'D', 'B', 'O', 'D', 'Y', ' ', 1.0f)) finalColor = float3(1, 1, 1);
                    else if (comp == 1 && Str10(px, float2(modMin.x + 30.0f, itemY + 10.0f), 'B', 'O', 'X', ' ', 'C', 'O', 'L', 'L', 'I', 'D', 1.0f)) finalColor = float3(1, 1, 1);
                    else if (comp == 2 && Str10(px, float2(modMin.x + 30.0f, itemY + 10.0f), 'A', 'U', 'D', 'I', 'O', ' ', 'S', 'O', 'U', 'R', 1.0f)) finalColor = float3(1, 1, 1);
                    else if (comp == 3 && Str10(px, float2(modMin.x + 30.0f, itemY + 10.0f), 'S', 'C', 'R', 'I', 'P', 'T', ' ', 'C', 'O', 'M', 1.0f)) finalColor = float3(1, 1, 1);
                    else if (comp == 4 && Str10(px, float2(modMin.x + 30.0f, itemY + 10.0f), 'P', 'A', 'R', 'T', 'I', 'C', 'L', 'E', ' ', 'E', 1.0f)) finalColor = float3(1, 1, 1);
                }
            } else { // Build Success Items
                if (Str10(px, float2(modMin.x + 30.0f, modMin.y + 60.0f), 'P', 'R', 'O', 'J', 'E', 'C', 'T', ' ', 'P', 'A', 1.0f)) finalColor = float3(0.3f, 0.9f, 0.5f);
                if (Str10(px, float2(modMin.x + 30.0f, modMin.y + 90.0f), 'D', 'I', 'S', 'T', ':', ' ', 'D', 'I', 'S', 'T', 1.0f)) finalColor = float3(0.8f, 0.85f, 0.95f);
                if (Str10(px, float2(modMin.x + 30.0f, modMin.y + 120.0f), 'S', 'T', 'A', 'N', 'D', 'A', 'L', 'O', 'N', 'E', 1.0f)) finalColor = float3(0.8f, 0.85f, 0.95f);

                float2 btnMin = float2(modMin.x + 30.0f, modMax.y - 60.0f);
                float2 btnMax = float2(modMin.x + 290.0f, modMax.y - 20.0f);
                if (px.x >= btnMin.x && px.x <= btnMax.x && px.y >= btnMin.y && px.y <= btnMax.y) {
                    finalColor = float3(0.18f, 0.65f, 0.35f);
                    if (Str10(px, float2(btnMin.x + 20.0f, btnMin.y + 12.0f), 'L', 'A', 'U', 'N', 'C', 'H', ' ', 'G', 'A', 'M', 1.0f)) finalColor = float3(1, 1, 1);
                }

                float2 closeMin = float2(modMax.x - 140.0f, modMax.y - 60.0f);
                float2 closeMax = float2(modMax.x - 30.0f, modMax.y - 20.0f);
                if (px.x >= closeMin.x && px.x <= closeMax.x && px.y >= closeMin.y && px.y <= closeMax.y) {
                    finalColor = float3(0.25f, 0.28f, 0.38f);
                    if (Str5(px, float2(closeMin.x + 28.0f, closeMin.y + 12.0f), 'C', 'L', 'O', 'S', 'E', 1.0f)) finalColor = float3(1, 1, 1);
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

            if (itemIdx == 0 && Str9(px, float2(dropMin.x + 14.0f, dropMin.y + 7.0f), 'N', 'E', 'W', ' ', 'S', 'C', 'E', 'N', 'E', 1.0f)) finalColor = float3(1, 1, 1);
            if (itemIdx == 1 && Str10(px, float2(dropMin.x + 14.0f, dropMin.y + 35.0f), 'O', 'P', 'E', 'N', ' ', 'S', 'C', 'E', 'N', 'E', 1.0f)) finalColor = float3(1, 1, 1);
            if (itemIdx == 2 && Str10(px, float2(dropMin.x + 14.0f, dropMin.y + 63.0f), 'S', 'A', 'V', 'E', ' ', 'S', 'C', 'E', 'N', 'E', 1.0f)) finalColor = float3(1, 1, 1);
            if (itemIdx == 3 && Str4(px, float2(dropMin.x + 14.0f, dropMin.y + 91.0f), 'E', 'X', 'I', 'T', 1.0f)) finalColor = float3(1.0f, 0.4f, 0.4f);
        }
    } else if (activeMenu == 2) { // EDIT
        float2 dropMin = float2(130.0f, 32.0f);
        float2 dropMax = float2(260.0f, 116.0f);
        if (px.x >= dropMin.x && px.x <= dropMax.x && px.y >= dropMin.y && px.y <= dropMax.y) {
            float inDropY = px.y - dropMin.y;
            int itemIdx = (int)(inDropY / 28.0f);
            finalColor = frac(inDropY / 28.0f) < 0.95f ? float3(0.19f, 0.22f, 0.31f) : float3(0.14f, 0.15f, 0.20f);
            if (itemIdx == 0 && Str4(px, dropMin + float2(14.0f, 7.0f), 'U', 'N', 'D', 'O', 1.0f)) finalColor = float3(1, 1, 1);
            if (itemIdx == 1 && Str4(px, dropMin + float2(14.0f, 35.0f), 'R', 'E', 'D', 'O', 1.0f)) finalColor = float3(1, 1, 1);
            if (itemIdx == 2 && Str6(px, dropMin + float2(14.0f, 63.0f), 'D', 'E', 'L', 'E', 'T', 'E', 1.0f)) finalColor = float3(1.0f, 0.62f, 0.62f);
        }
    } else if (activeMenu == 3) { // SCENE
        float2 dropMin = float2(180.0f, 32.0f);
        float2 dropMax = float2(320.0f, 116.0f);
        if (px.x >= dropMin.x && px.x <= dropMax.x && px.y >= dropMin.y && px.y <= dropMax.y) {
            float inDropY = px.y - dropMin.y;
            int itemIdx = (int)(inDropY / 28.0f);
            finalColor = frac(inDropY / 28.0f) < 0.95f ? float3(0.19f, 0.22f, 0.31f) : float3(0.14f, 0.15f, 0.20f);
            if (itemIdx == 0 && Str8(px, dropMin + float2(14.0f, 7.0f), 'A', 'D', 'D', ' ', 'C', 'U', 'B', 'E', 1.0f)) finalColor = float3(1, 1, 1);
            if (itemIdx == 1 && Str10(px, dropMin + float2(14.0f, 35.0f), 'A', 'D', 'D', ' ', 'S', 'P', 'H', 'E', 'R', 'E', 1.0f)) finalColor = float3(1, 1, 1);
            if (itemIdx == 2 && Str10(px, dropMin + float2(14.0f, 63.0f), 'A', 'D', 'D', ' ', 'C', 'Y', 'L', 'I', 'N', 'D', 1.0f)) finalColor = float3(1, 1, 1);
        }
    } else if (activeMenu == 4) { // BUILD
        float2 dropMin = float2(240.0f, 32.0f);
        float2 dropMax = float2(420.0f, 32.0f + 28.0f);
        if (px.x >= dropMin.x && px.x <= dropMax.x && px.y >= dropMin.y && px.y <= dropMax.y) {
            finalColor = IsHovered(px, dropMin, dropMax) ? float3(0.20f, 0.25f, 0.34f) : float3(0.14f, 0.15f, 0.20f);
            if (Str8(px, float2(dropMin.x + 14.0f, dropMin.y + 7.0f), 'P', 'A', 'C', 'K', 'A', 'G', 'E', 1.0f)) finalColor = float3(0.64f, 0.91f, 0.72f);
        }
    }

    return float4(finalColor, 1.0f);
}
