#pragma once

#include <cstdint>

namespace AMC {

    // ================================================================
    // Lightweight version-counter system for invalidation.
    //
    // Producers bump a version when relevant state changes.
    // Consumers cache the last-processed version and skip work
    // when it matches.
    //
    // All counters start at 1 so that a consumer initialized to 0
    // is guaranteed to process on the first frame.
    // ================================================================

    struct SceneVersions {
        // Bumped when any instance transform, visibility, or animation changes
        uint64_t geometryVersion = 1;

        // Bumped when any light property changes (position, color, active, shadow config)
        uint64_t lightVersion = 1;

        // Combined: max(geometryVersion, lightVersion) for consumers that care about both
        uint64_t rasterShadowVersion() const { return geometryVersion + lightVersion; }

        // Bumped when voxelization inputs change (geometry or light)
        uint64_t voxelVersion() const { return geometryVersion + lightVersion; }
    };

    // Snapshot of runtime toggles that affect whether cached data is required.
    // When any toggle changes, affected caches must be invalidated.
    struct RenderConfigSnapshot {
        bool isVXGI = false;
        bool isVolumetric = false;
        bool isGenerateShadowMaps = true;
        bool deferredWantsRTShadows = true;
        bool isRTShadows = false;

        bool operator!=(const RenderConfigSnapshot& o) const {
            return isVXGI != o.isVXGI || isVolumetric != o.isVolumetric ||
                   isGenerateShadowMaps != o.isGenerateShadowMaps ||
                   deferredWantsRTShadows != o.deferredWantsRTShadows ||
                   isRTShadows != o.isRTShadows;
        }
    };

    // Per-subsystem cached version, used by consumers to detect staleness
    struct VersionCache {
        uint64_t lastGeometryVersion  = 0;
        uint64_t lastLightVersion     = 0;
        uint64_t lastExtractorVersion = 0;
        uint64_t lastShadowVersion    = 0;
        uint64_t lastVoxelVersion     = 0;
        uint64_t lastTlasVersion      = 0;
        RenderConfigSnapshot lastConfig;
    };

    // ================================================================
    // FrameDirtyFlags
    //
    // Computed once per frame from version comparisons.
    // Passes read these to decide whether to skip work.
    // ================================================================

    struct FrameDirtyFlags {
        bool geometryDirty    = true;  // instance transforms or visibility changed
        bool lightDirty       = true;  // light properties changed
        bool extractorDirty   = true;  // need to rebuild render items
        bool rasterShadowDirty = true; // need to re-render raster shadow maps
        bool voxelDirty       = true;  // need to re-voxelize
        bool tlasDirty        = true;  // need to rebuild TLAS
    };

} // namespace AMC
