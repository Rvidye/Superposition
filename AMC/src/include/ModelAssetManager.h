#pragma once

#include <common.h>
#include <Model.h>
#include <UBO.h>

namespace AMC {

	// ================================================================
	// CPU-side mirrors of GpuTypes.glsl structs
	// ================================================================

	struct GpuMeshRecord {
		uint32_t meshletOffset;
		uint32_t meshletCount;
		uint32_t materialId;
		float    _pad0 = 0.0f;
		glm::vec3 boundsMin;
		float    _pad1 = 0.0f;
		glm::vec3 boundsMax;
		float    _pad2 = 0.0f;
	};

	struct GpuNodeRecord {
		int32_t  parentIndex;          // -1 for root
		uint32_t meshBindingOffset;
		uint32_t meshBindingCount;
		float    _pad0 = 0.0f;
		glm::mat4 localTransform;
	};

	struct GpuMeshBinding {
		uint32_t meshId;
	};

	struct GpuModelAssetRecord {
		uint32_t nodeOffset;
		uint32_t nodeCount;
		uint32_t meshOffset;
		uint32_t meshCount;
		uint32_t meshBindingOffset;
		uint32_t meshBindingCount;
		uint32_t materialOffset;
		uint32_t materialCount;
	};

	// FlatNode is defined in Model.h

	// ================================================================
	// ModelAssetManager
	//
	// Owns all immutable global GPU pools. Models register their data
	// here at load time and receive stable offsets/IDs back.
	// ================================================================

	class ModelAssetManager {
	public:
		static ModelAssetManager& Get();

		// Register a loaded model's data into global pools.
		// Returns the assetId assigned to this model.
		// After this call, Model's pool offsets are filled in.
		uint32_t RegisterModel(Model* model);

		// Create GPU buffers and upload all accumulated data.
		// Call once after all models are loaded.
		void UploadToGPU();

		// Bind all immutable global SSBOs to their shader binding points.
		void BindGlobalBuffers() const;

		// Query
		uint32_t GetAssetCount() const { return (uint32_t)m_assets.size(); }
		const GpuModelAssetRecord& GetAsset(uint32_t assetId) const { return m_assets[assetId]; }

		// Global pool accessors (CPU-side, for building render items etc.)
		const std::vector<GpuMeshRecord>& GetMeshRecords() const { return m_meshRecords; }
		const std::vector<GpuNodeRecord>& GetNodeRecords() const { return m_nodeRecords; }
		const std::vector<GpuMeshBinding>& GetMeshBindings() const { return m_meshBindings; }

		// Allow LoadMaterials to push materials into the global pool before RegisterModel
		void AppendMaterials(const GPUMaterial* data, uint32_t count) {
			m_globalMaterials.insert(m_globalMaterials.end(), data, data + count);
		}
		uint32_t GetMaterialPoolSize() const { return (uint32_t)m_globalMaterials.size(); }

	private:
		ModelAssetManager() = default;

		// Flatten a recursive NodeData tree into the flat arrays
		void FlattenNodeHierarchy(
			const NodeData& node,
			int32_t parentIndex,
			std::vector<FlatNode>& outFlatNodes
		);

		// === CPU-side pool data (accumulated during registration) ===

		// Per-asset metadata
		std::vector<GpuModelAssetRecord> m_assets;

		// Global vertex pool (Vertex structs, same layout as current)
		std::vector<Vertex> m_globalVertices;

		// Global meshlet pools
		std::vector<GpuMeshlet>    m_globalMeshlets;
		std::vector<GpuMeshletInfo> m_globalMeshletInfos;
		std::vector<uint32_t>      m_globalMeshletVertexIndices;
		std::vector<uint32_t>      m_globalMeshletLocalIndices;

		// Global material pool (GPUMaterial from UBO.h)
		std::vector<GPUMaterial>   m_globalMaterials;

		// Global mesh/node/binding records
		std::vector<GpuMeshRecord>   m_meshRecords;
		std::vector<GpuNodeRecord>   m_nodeRecords;
		std::vector<GpuMeshBinding>  m_meshBindings;

		// === GPU buffer handles ===
		GLuint m_vertexPoolSSBO        = 0;
		GLuint m_meshletPoolSSBO       = 0;
		GLuint m_meshletInfoPoolSSBO   = 0;
		GLuint m_meshletVertexPoolSSBO = 0;
		GLuint m_meshletLocalPoolSSBO  = 0;
		GLuint m_materialPoolSSBO      = 0;
		GLuint m_meshRecordPoolSSBO    = 0;
		GLuint m_nodeRecordPoolSSBO    = 0;
		GLuint m_meshBindingPoolSSBO   = 0;
		GLuint m_assetPoolSSBO         = 0;

		bool m_uploaded = false;
	};

	// ================================================================
	// SSBO binding points for global pools
	// These must match the shader-side declarations
	// ================================================================

	namespace GlobalBindings {
		// Existing bindings we keep:
		// SSBO 2  = materialSSBO (we reuse this slot for global material pool)
		// SSBO 8  = vertexDataSSBO (we reuse this slot for global vertex pool)
		// SSBO 11 = meshletSSBO (reuse for global meshlet pool)
		// SSBO 12 = meshletInfoSSBO (reuse for global meshlet info pool)
		// SSBO 13 = meshletVertexIndicesSSBO (reuse for global meshlet vertex pool)
		// SSBO 14 = meshletLocalIndicesSSBO (reuse for global meshlet local pool)
		//
		// New bindings:
		constexpr GLuint MeshRecordPool    = 15;
		constexpr GLuint NodeRecordPool    = 16;
		constexpr GLuint MeshBindingPool   = 17;
		constexpr GLuint AssetPool         = 18;
		constexpr GLuint TransformPalette  = 19;  // current transforms (Milestone 2)
		constexpr GLuint PrevTransformPalette = 20; // previous transforms (Milestone 2)
		constexpr GLuint InstancePool      = 21;  // GpuModelInstance (Milestone 2)
		constexpr GLuint RenderItemPool    = 22;  // GpuRenderItem (Milestone 4)
		constexpr GLuint MeshTaskCmdPool   = 23;  // GpuMeshTaskCommand (Milestone 5)
	}
}
