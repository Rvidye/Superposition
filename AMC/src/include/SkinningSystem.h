#pragma once

#include <common.h>
#include <Model.h>
#include <ModelAssetManager.h>
#include <SceneInstanceManager.h>

namespace AMC {

	// ================================================================
	// SkinningSystem
	//
	// Manages global bone palettes and skinned vertex pools.
	// Skeletal models get their bone transforms evaluated on CPU,
	// uploaded to a global bone palette, then GPU compute skinning
	// writes to global current/previous skinned vertex buffers.
	//
	// For the mesh shader path, skinned vertices replace the
	// unskinned vertices in the global vertex pool at render time.
	// Until the mesh shader path for skeletal models is fully wired,
	// skeletal models fall back to the traditional VAO draw path.
	// ================================================================

	struct SkinningRecord {
		uint32_t instanceId;
		uint32_t boneOffset;           // into global bone palette
		uint32_t boneCount;
		uint32_t skinnedVertexOffset;  // into global skinned vertex pool
		uint32_t skinnedVertexCount;
		Model*   model;
	};

	class SkinningSystem {
	public:
		static SkinningSystem& Get();

		// Register a skeletal model instance. Call after SceneInstanceManager
		// creates the instance.
		void RegisterSkeletalInstance(uint32_t instanceId, Model* model);

		// Per-frame: evaluate bone transforms, upload, dispatch compute skinning.
		// Call after SceneInstanceManager::BeginFrame.
		void Update(float deltaTime);

		// Bind skinned vertex pool SSBOs if needed
		void BindBuffers() const;

		// Check if a model instance has skinning active
		bool HasSkinning(uint32_t instanceId) const;

		// Get the skinned vertex offset for an instance (for mesh shader path)
		uint32_t GetSkinnedVertexOffset(uint32_t instanceId) const;

		// True if any visible skeletal instance was animated this frame
		bool IsSkeletalDirtyThisFrame() const { return m_skeletalDirtyThisFrame; }

	private:
		SkinningSystem() = default;

		void EvaluateBones(SkinningRecord& rec, float deltaTime);

		std::vector<SkinningRecord> m_records;
		std::unordered_map<uint32_t, uint32_t> m_instanceToRecord;

		// Global bone palette (CPU side)
		std::vector<glm::mat4> m_currentBones;
		std::vector<glm::mat4> m_prevBones;
		uint32_t m_nextBoneSlot = 0;

		// Global skinned vertex pools
		uint32_t m_nextSkinnedVertexSlot = 0;

		// GPU buffers
		GLuint m_currentBoneSSBO = 0;
		GLuint m_prevBoneSSBO = 0;
		GLuint m_currentSkinnedVertexSSBO = 0;
		GLuint m_prevSkinnedVertexSSBO = 0;

		ShaderProgram* m_skinComputeProgram = nullptr;
		bool m_gpuCreated = false;
		bool m_skeletalDirtyThisFrame = false;
	};

} // namespace AMC
