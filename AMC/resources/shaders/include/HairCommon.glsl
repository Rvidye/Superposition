const uint HAIR_MAX_SEGMENTS = 16u;
const uint HAIR_MAX_CENTER_COUNT = HAIR_MAX_SEGMENTS + 1u;
const float HAIR_TWO_PI = 6.28318530718;

struct HairLodSelection
{
    uint RenderStep;
    uint ParentStep;
    uint RenderSegmentCount;
    uint RenderCenterCount;
    float MorphGamma;
};

float HairSliceCoord(int sliceIndex, int depth)
{
    return (float(sliceIndex) + 0.5) / float(depth);
}

vec3 HairSafeNormalize(vec3 value, vec3 fallback)
{
    float len2 = dot(value, value);
    if (len2 <= 1e-8)
    {
        return fallback;
    }
    return value * inversesqrt(len2);
}

vec2 HairRootToAtlasUV(HairBundleDesc bundle, vec2 rootUV)
{
    ivec3 texSize = textureSize(hairAssetUBO.hairAsset.HairMeshTexture, 0);
    vec2 texelMin = bundle.AtlasRect.xy + vec2(0.5);
    vec2 texelMax = bundle.AtlasRect.xy + bundle.AtlasRect.zw - vec2(0.5);
    return mix(texelMin / vec2(texSize.xy), texelMax / vec2(texSize.xy), clamp(rootUV, vec2(0.0), vec2(1.0)));
}

vec2 HairGlobalToAtlasUV(vec2 globalUV)
{
    float bundlesPerAxis = max(float(hairAssetUBO.hairAsset.Counts.y), 1.0);
    ivec3 texSize = textureSize(hairAssetUBO.hairAsset.HairMeshTexture, 0);
    vec2 clampedUV = clamp(globalUV, vec2(0.0), vec2(0.999999));
    vec2 scaledUV = clampedUV * bundlesPerAxis;
    vec2 bundleCoord = floor(scaledUV);
    vec2 localUV = fract(scaledUV);
    vec2 texelMin = bundleCoord * 2.0 + vec2(0.5);
    vec2 texelMax = texelMin + vec2(1.0);
    return mix(texelMin / vec2(texSize.xy), texelMax / vec2(texSize.xy), localUV);
}

float HairStyleCoord(float wCoord)
{
    int depth = textureSize(hairAssetUBO.hairAsset.WTexture, 0).z;
    return clamp((clamp(wCoord, 0.0, 1.0) * float(depth - 1) + 0.5) / float(depth), 0.0, 1.0);
}

vec3 HairSampleBetweenSlices(vec2 atlasUV, int sliceA, int sliceB, float t)
{
    int depth = textureSize(hairAssetUBO.hairAsset.HairMeshTexture, 0).z;
    float sliceCoord = mix(HairSliceCoord(sliceA, depth), HairSliceCoord(sliceB, depth), clamp(t, 0.0, 1.0));
    return texture(hairAssetUBO.hairAsset.HairMeshTexture, vec3(atlasUV, sliceCoord)).xyz;
}

vec3 HairEvaluateLocalCenterFromAtlasUV(vec2 atlasUV, float t)
{
    int tipSlice = hairAssetUBO.hairAsset.Counts.x * 4;
    int midSlice = tipSlice / 2;

    if (t <= 0.5)
    {
        float localT = t * 2.0;
        vec3 a = HairSampleBetweenSlices(atlasUV, 0, 1, localT);
        vec3 b = HairSampleBetweenSlices(atlasUV, 1, midSlice, localT);
        return mix(a, b, localT);
    }

    float localT = (t - 0.5) * 2.0;
    vec3 a = HairSampleBetweenSlices(atlasUV, midSlice, tipSlice - 1, localT);
    vec3 b = HairSampleBetweenSlices(atlasUV, tipSlice - 1, tipSlice, localT);
    return mix(a, b, localT);
}

vec3 HairEvaluateLocalCenter(HairBundleDesc bundle, vec2 rootUV, float t)
{
    return HairEvaluateLocalCenterFromAtlasUV(HairRootToAtlasUV(bundle, rootUV), t);
}

vec2 HairSampleGlobalUV(vec2 atlasUV)
{
    return texture(hairAssetUBO.hairAsset.UVTexture, atlasUV).xy;
}

float HairSampleW(vec2 atlasUV, float wCoord)
{
    return texture(hairAssetUBO.hairAsset.WTexture, vec3(atlasUV, HairStyleCoord(wCoord))).r;
}

mat3 HairSampleFrame(vec2 atlasUV, float wCoord)
{
    vec3 uDir = HairSafeNormalize(texture(hairAssetUBO.hairAsset.UTexture, vec3(atlasUV, HairStyleCoord(wCoord))).xyz, vec3(1.0, 0.0, 0.0));
    vec3 vDir = HairSafeNormalize(texture(hairAssetUBO.hairAsset.VTexture, vec3(atlasUV, HairStyleCoord(wCoord))).xyz, vec3(0.0, 0.0, 1.0));
    vec3 wDir = HairSafeNormalize(cross(uDir, vDir), vec3(0.0, 1.0, 0.0));
    uDir = HairSafeNormalize(uDir - wDir * dot(uDir, wDir), uDir);
    vDir = HairSafeNormalize(cross(wDir, uDir), vDir);
    return mat3(uDir, vDir, wDir);
}

float HairHash11(float p)
{
    return fract(sin(p * 127.1) * 43758.5453123);
}

float HairHash31(vec3 p)
{
    return fract(sin(dot(p, vec3(127.1, 311.7, 74.7))) * 43758.5453123);
}

float HairValueNoise3(vec3 p)
{
    vec3 cell = floor(p);
    vec3 fractPart = fract(p);
    vec3 smoothPart = fractPart * fractPart * (3.0 - 2.0 * fractPart);

    float n000 = HairHash31(cell + vec3(0.0, 0.0, 0.0));
    float n100 = HairHash31(cell + vec3(1.0, 0.0, 0.0));
    float n010 = HairHash31(cell + vec3(0.0, 1.0, 0.0));
    float n110 = HairHash31(cell + vec3(1.0, 1.0, 0.0));
    float n001 = HairHash31(cell + vec3(0.0, 0.0, 1.0));
    float n101 = HairHash31(cell + vec3(1.0, 0.0, 1.0));
    float n011 = HairHash31(cell + vec3(0.0, 1.0, 1.0));
    float n111 = HairHash31(cell + vec3(1.0, 1.0, 1.0));

    float nx00 = mix(n000, n100, smoothPart.x);
    float nx10 = mix(n010, n110, smoothPart.x);
    float nx01 = mix(n001, n101, smoothPart.x);
    float nx11 = mix(n011, n111, smoothPart.x);
    float nxy0 = mix(nx00, nx10, smoothPart.y);
    float nxy1 = mix(nx01, nx11, smoothPart.y);
    return mix(nxy0, nxy1, smoothPart.z);
}

vec2 HairNoiseDirection2(vec3 p)
{
    const float eps = 0.125;
    vec2 gradient = vec2(
        HairValueNoise3(p + vec3(eps, 0.0, 0.0)) - HairValueNoise3(p - vec3(eps, 0.0, 0.0)),
        HairValueNoise3(p + vec3(0.0, eps, 0.0)) - HairValueNoise3(p - vec3(0.0, eps, 0.0))
    );

    if (dot(gradient, gradient) <= 1e-8)
    {
        float angle = HairHash31(p + vec3(17.0, 29.0, 41.0)) * HAIR_TWO_PI;
        gradient = vec2(cos(angle), sin(angle));
    }

    return normalize(gradient);
}

vec3 HairApplyFuzz(mat3 frame, float wCoord, vec2 strandSeeds)
{
    float amplitude = hairAssetUBO.hairAsset.FuzzParams.x
        * pow(max(wCoord, 0.0), hairAssetUBO.hairAsset.FuzzParams.y)
        * pow(max(strandSeeds.x, 1e-3), hairAssetUBO.hairAsset.FuzzParams.z);
    float angle = strandSeeds.y * HAIR_TWO_PI;
    return frame * vec3(cos(angle) * amplitude, sin(angle) * amplitude, 0.0);
}

vec3 HairApplyFrizz(mat3 frame, vec2 globalUV, float wCoord)
{
    vec2 direction = HairNoiseDirection2(vec3(globalUV * hairAssetUBO.hairAsset.FrizzParams.z, 0.0));
    float amplitude = hairAssetUBO.hairAsset.FrizzParams.x * pow(max(wCoord, 0.0), hairAssetUBO.hairAsset.FrizzParams.y);
    return frame * vec3(direction * amplitude, 0.0);
}

vec3 HairApplyKink(mat3 frame, vec2 globalUV, float wCoord)
{
    vec2 direction = HairNoiseDirection2(vec3(
        globalUV * hairAssetUBO.hairAsset.KinkParams.z,
        wCoord * hairAssetUBO.hairAsset.KinkParams.w
    ));
    float amplitude = hairAssetUBO.hairAsset.KinkParams.x * pow(max(wCoord, 0.0), hairAssetUBO.hairAsset.KinkParams.y);
    return frame * vec3(direction * amplitude, 0.0);
}

vec3 HairApplyCurl(mat3 frame, vec2 globalUV, float wCoord, float phaseSeed)
{
    float phase = HairValueNoise3(vec3(globalUV * hairAssetUBO.hairAsset.CurlParams.w, phaseSeed * 23.0));
    float theta = HAIR_TWO_PI * hairAssetUBO.hairAsset.StyleExtraParams.x
        * (phase + pow(max(wCoord, 0.0), hairAssetUBO.hairAsset.CurlParams.z));
    float amplitude = hairAssetUBO.hairAsset.CurlParams.x * pow(max(wCoord, 0.0), hairAssetUBO.hairAsset.CurlParams.y);
    return frame * vec3(cos(theta) * amplitude, sin(theta) * amplitude, 0.0);
}

vec3 HairApplyClump(vec3 baseCenter, vec2 globalUV, float strandT, float wCoord, float strandSeed)
{
    float gridResolution = max(hairAssetUBO.hairAsset.ClumpParams.y, 1.0);
    if (strandSeed > hairAssetUBO.hairAsset.ClumpParams.z)
    {
        return vec3(0.0);
    }

    vec2 cell = floor(clamp(globalUV, vec2(0.0), vec2(0.999999)) * gridResolution);
    vec2 cellMin = cell / gridResolution;
    vec2 cellMax = (cell + 1.0) / gridResolution;
    vec2 clumpCenterUV = (cell + 0.5) / gridResolution;

    vec2 clumpNoise = HairNoiseDirection2(vec3(clumpCenterUV * hairAssetUBO.hairAsset.ClumpParams.x, strandSeed * 19.0));
    clumpCenterUV += clumpNoise * (hairAssetUBO.hairAsset.StyleExtraParams.w / gridResolution);
    clumpCenterUV = clamp(clumpCenterUV, cellMin, cellMax);

    vec2 variedUV = mix(globalUV, clumpCenterUV, hairAssetUBO.hairAsset.ClumpParams.w * strandSeed);
    vec3 referenceCenter = HairEvaluateLocalCenterFromAtlasUV(HairGlobalToAtlasUV(variedUV), strandT);
    vec3 delta = referenceCenter - baseCenter;
    float distanceToCenter = length(delta);
    if (distanceToCenter <= 1e-6)
    {
        return vec3(0.0);
    }

    float h = clamp(length(globalUV - clumpCenterUV) * gridResolution, 0.0, 1.0);
    float influence = max(1.0 - h * hairAssetUBO.hairAsset.StyleExtraParams.y / distanceToCenter, 0.0);
    return delta * influence * pow(max(wCoord, 0.0), hairAssetUBO.hairAsset.StyleExtraParams.z);
}

vec3 HairEvaluateStyledLocalCenter(HairBundleDesc bundle, vec2 rootUV, vec2 strandSeeds, float strandT)
{
    vec3 baseCenter = HairEvaluateLocalCenter(bundle, rootUV, strandT);
    if (strandT <= 0.0)
    {
        return baseCenter;
    }

    vec2 atlasUV = HairRootToAtlasUV(bundle, rootUV);
    vec2 globalUV = HairSampleGlobalUV(atlasUV);
    float wCoord = HairSampleW(atlasUV, strandT);
    mat3 frame = HairSampleFrame(atlasUV, strandT);

    vec3 styledCenter = baseCenter;
    styledCenter += HairApplyClump(baseCenter, globalUV, strandT, wCoord, strandSeeds.x);
    styledCenter += HairApplyFuzz(frame, wCoord, strandSeeds);
    styledCenter += HairApplyFrizz(frame, globalUV, wCoord);
    styledCenter += HairApplyKink(frame, globalUV, wCoord);
    styledCenter += HairApplyCurl(frame, globalUV, wCoord, strandSeeds.y);
    return styledCenter;
}

float HairComputeBundleLodFactor(vec3 boundsMin, vec3 boundsMax, vec3 viewPos, mat4 projection)
{
    vec3 center = (boundsMin + boundsMax) * 0.5;
    vec3 extents = (boundsMax - boundsMin) * 0.5;
    float radius = max(length(extents), 1e-3);
    float distanceToBundle = max(length(viewPos - center), 1e-3);
    float tanHalfFov = max(1.0 / max(projection[1][1], 1e-4), 1e-4);
    float lod = radius / (distanceToBundle * tanHalfFov);
    lod *= hairAssetUBO.hairAsset.LODParams.y;
    return clamp(lod, hairAssetUBO.hairAsset.LODParams.w, 1.0);
}

uint HairComputeBundleLodCount(uint bundleIndex, uint baseCount, float lodFactor)
{
    float jitter = HairHash11(float(bundleIndex) * 17.0 + 3.0);
    float desiredCount = lodFactor * (float(baseCount) + jitter);
    return uint(clamp(floor(desiredCount), 1.0, float(baseCount)));
}

HairLodSelection HairSelectSegmentLod(float lodFactor)
{
    HairLodSelection selection;
    uint baseSegments = uint(hairAssetUBO.hairAsset.Counts.w);
    float scaledSegments = max(float(baseSegments) * lodFactor, 1.0);
    float lodExponent = log2(scaledSegments);
    float coarseExponent = floor(lodExponent);
    uint coarseSegments = uint(exp2(coarseExponent));
    uint fineSegments = min(coarseSegments * 2u, baseSegments);
    float transitionWindow = max(hairAssetUBO.hairAsset.LODParams.z, 1e-4);

    selection.RenderSegmentCount = coarseSegments;
    selection.RenderStep = baseSegments / coarseSegments;
    selection.ParentStep = selection.RenderStep;
    selection.MorphGamma = 0.0;

    if (fineSegments != coarseSegments)
    {
        float transitionStart = coarseExponent;
        float transitionEnd = coarseExponent + transitionWindow;
        if (lodExponent > transitionStart)
        {
            selection.RenderSegmentCount = fineSegments;
            selection.RenderStep = baseSegments / fineSegments;
            selection.ParentStep = baseSegments / coarseSegments;
            if (lodExponent < transitionEnd)
            {
                selection.MorphGamma = 1.0 - clamp((lodExponent - transitionStart) / transitionWindow, 0.0, 1.0);
            }
            else
            {
                selection.ParentStep = selection.RenderStep;
            }
        }
    }

    selection.RenderCenterCount = selection.RenderSegmentCount + 1u;
    return selection;
}

float HairComputeThicknessScale(float lodFactor)
{
    return clamp(1.0 / max(lodFactor, hairAssetUBO.hairAsset.LODParams.w), 1.0, 4.0);
}

vec3 HairComputeLocalTangent(in vec3 centers[17], uint centerCount, uint centerIndex)
{
    if (centerIndex == 0u)
    {
        vec3 q = HairSafeNormalize(centers[1] - centers[0], vec3(0.0, 1.0, 0.0));
        vec3 nextTangent = HairSafeNormalize(centers[2] - centers[0], q);
        return HairSafeNormalize(2.0 * q * dot(nextTangent, q) - nextTangent, q);
    }

    if (centerIndex == centerCount - 1u)
    {
        vec3 q = HairSafeNormalize(centers[centerCount - 1u] - centers[centerCount - 2u], vec3(0.0, 1.0, 0.0));
        vec3 prevTangent = HairSafeNormalize(centers[centerCount - 1u] - centers[centerCount - 3u], q);
        return HairSafeNormalize(2.0 * q * dot(prevTangent, q) - prevTangent, q);
    }

    return HairSafeNormalize(centers[centerIndex + 1u] - centers[centerIndex - 1u], vec3(0.0, 1.0, 0.0));
}

vec3 HairComputeBillboardAxis(vec3 tangent, vec3 referenceDir)
{
    vec3 ribbon = cross(tangent, referenceDir);
    if (dot(ribbon, ribbon) <= 1e-8)
    {
        vec3 fallbackAxis = abs(tangent.y) < 0.99 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
        ribbon = cross(tangent, fallbackAxis);
    }
    return HairSafeNormalize(ribbon, vec3(1.0, 0.0, 0.0));
}

uint HairPrimitiveIndexAt(uint byteIndex)
{
    uint triangleIndex = byteIndex / 3u;
    uint corner = byteIndex % 3u;
    uint segmentIndex = triangleIndex / 2u;
    uint baseVertex = segmentIndex * 2u;

    if ((triangleIndex & 1u) == 0u)
    {
        return (corner == 0u) ? baseVertex : ((corner == 1u) ? baseVertex + 1u : baseVertex + 2u);
    }

    return (corner == 0u) ? baseVertex + 1u : ((corner == 1u) ? baseVertex + 3u : baseVertex + 2u);
}
