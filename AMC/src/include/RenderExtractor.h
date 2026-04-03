#pragma once

#include <common.h>
#include <ModelAssetManager.h>
#include <SceneInstanceManager.h>

namespace AMC {

	// ================================================================
	// CPU-side mirrors of GpuTypes.glsl render structs
	// ================================================================

	struct GpuRenderItem {
		uint32_t  meshId;            // global mesh pool index
		uint32_t  transformIndex;    // = instance.transformBase + nodeIndex
		uint32_t  materialId;        // global material pool index
		uint32_t  instanceId;        // back-reference to GpuModelInstance
		glm::vec3 worldBoundsMin;
		float     _pad0 = 0.0f;
		glm::vec3 worldBoundsMax;
		float     _pad1 = 0.0f;
	};

	struct GpuMeshTaskCommand {
		uint32_t count;              // number of task shader workgroups
		uint32_t first;              // always 0 for NV path
		uint32_t renderItemId;       // into render item pool
		uint32_t _pad0 = 0;
	};

	// ================================================================
	// RenderExtractor
	//
	// Builds flat GpuRenderItem[] and GpuMeshTaskCommand[] arrays
	// from all visible, mesh-shader-eligible instances.
	// Skeletal instances are excluded (they use the VAO path).
	//
	// Call Extract() once per frame after SceneInstanceManager::BeginFrame,
	// then UploadToGPU() before rendering.
	// ================================================================

	class RenderExtractor {
	public:
		static RenderExtractor& Get();

		// Build render items + commands from all visible instances.
		void Extract();

		// Upload render items + commands to GPU SSBOs.
		void UploadToGPU();

		// Bind render item + command SSBOs to their binding points.
		void BindBuffers() const;

		// Get the command count for indirect dispatch
		uint32_t GetCommandCount() const { return (uint32_t)m_commands.size(); }

		// Get the indirect command buffer handle (also used as GL_DRAW_INDIRECT_BUFFER)
		GLuint GetIndirectBuffer() const { return m_commandSSBO; }

		// Get the count buffer for glMultiDrawMeshTasksIndirectCountNV
		GLuint GetCountBuffer() const { return m_countBuffer; }

		bool HasItems() const { return !m_renderItems.empty(); }

		// Version-aware extract: returns true if extraction was performed
		bool ExtractIfDirty(uint64_t geometryVersion);
		uint64_t GetLastExtractedVersion() const { return m_lastGeometryVersion; }

	private:
		RenderExtractor() = default;

		std::vector<GpuRenderItem> m_renderItems;
		std::vector<GpuMeshTaskCommand> m_commands;

		GLuint m_renderItemSSBO = 0;
		GLuint m_commandSSBO = 0;
		GLuint m_countBuffer = 0;
		bool m_gpuCreated = false;
		uint64_t m_lastGeometryVersion = 0;
	};

} // namespace AMC
