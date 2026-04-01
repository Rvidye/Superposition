#pragma once

#include <common.h>
#include <Model.h>
#include <ModelAssetManager.h>

namespace AMC {

	// ================================================================
	// Instance flags (matches GpuModelInstance.flags in GpuTypes.glsl)
	// ================================================================
	namespace InstanceFlags {
		constexpr uint32_t Visible  = 1 << 0;
		constexpr uint32_t Animated = 1 << 1;
		constexpr uint32_t Skeletal = 1 << 2;
	}

	// ================================================================
	// CPU-side instance record
	// ================================================================
	struct InstanceRecord {
		uint32_t    assetId;
		uint32_t    transformBase;      // offset into the transform palette
		uint32_t    nodeCount;
		uint32_t    flags;
		Model*      model;              // back-pointer for animation data
		glm::mat4   worldMatrix;        // model/instance world matrix
		glm::mat4   prevWorldMatrix;    // previous frame world matrix
		int32_t     activeAnimation;    // -1 = none
		std::string sceneName;          // key in Scene::models map
	};

	// ================================================================
	// GpuModelInstance - matches GLSL layout
	// ================================================================
	struct GpuModelInstance {
		uint32_t assetId;
		uint32_t transformBase;
		uint32_t nodeCount;
		uint32_t flags;
	};

	// ================================================================
	// SceneInstanceManager
	//
	// Owns live model instances and their transform palettes.
	// Transform palette layout:
	//   transformPalette[instance.transformBase + nodeIndex] = worldTransform
	//
	// Two palettes: current frame and previous frame (for velocity).
	// ================================================================
	class SceneInstanceManager {
	public:
		static SceneInstanceManager& Get();

		// Create an instance of a registered model asset.
		// Returns instanceId.
		uint32_t CreateInstance(Model* model, const glm::mat4& worldMatrix, const std::string& sceneName);

		// Remove an instance (frees its transform range).
		// For now, does not compact — just marks as unused.
		void RemoveInstance(uint32_t instanceId);

		// Set instance visibility
		void SetVisible(uint32_t instanceId, bool visible);

		// Set instance world matrix
		void SetWorldMatrix(uint32_t instanceId, const glm::mat4& matrix);

		// === Per-frame operations ===

		// Step 1: Copy current transforms to previous, then evaluate
		//         animation and world transforms into current palette.
		//         Call this once per frame before rendering.
		void BeginFrame(float deltaTime);

		// Step 2: Upload current and previous palettes + instance data to GPU.
		void UploadToGPU();

		// Step 3: Bind transform/instance SSBOs.
		void BindBuffers() const;

		// === Accessors ===
		uint32_t GetInstanceCount() const { return (uint32_t)m_instances.size(); }
		const InstanceRecord& GetInstance(uint32_t id) const { return m_instances[id]; }
		InstanceRecord& GetInstance(uint32_t id) { return m_instances[id]; }

		// Read current world transform for a specific node of an instance
		const glm::mat4& GetCurrentTransform(uint32_t instanceId, uint32_t nodeIndex) const;
		const glm::mat4& GetPrevTransform(uint32_t instanceId, uint32_t nodeIndex) const;

		// Total size of transform palette
		uint32_t GetPaletteSize() const { return m_nextTransformSlot; }

	private:
		SceneInstanceManager() = default;

		// Evaluate node animation for one instance into the current palette
		void EvaluateNodeAnimation(InstanceRecord& inst, float deltaTime);

		// Propagate world transforms through the flat node hierarchy
		void PropagateTransforms(InstanceRecord& inst);

		// === Instance storage ===
		std::vector<InstanceRecord> m_instances;

		// === Transform palettes (CPU-side) ===
		// Indexed by [transformBase + nodeIndex]
		std::vector<glm::mat4> m_currentTransforms;
		std::vector<glm::mat4> m_prevTransforms;

		// Flat local transforms (working buffer for animation)
		// Same indexing as palettes
		std::vector<glm::mat4> m_localTransforms;

		// Next available slot in the palette
		uint32_t m_nextTransformSlot = 0;

		// === GPU-side instance data ===
		std::vector<GpuModelInstance> m_gpuInstances;

		// === GPU buffer handles ===
		GLuint m_currentTransformSSBO = 0;
		GLuint m_prevTransformSSBO    = 0;
		GLuint m_instanceSSBO         = 0;

		bool m_gpuBuffersCreated = false;
	};

} // namespace AMC
