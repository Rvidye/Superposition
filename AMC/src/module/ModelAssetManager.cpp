#include <ModelAssetManager.h>
#include <UBO.h>
#include <meshoptimizer/meshoptimizer.h>

namespace AMC {

	ModelAssetManager& ModelAssetManager::Get() {
		static ModelAssetManager instance;
		return instance;
	}

	// ================================================================
	// FlattenNodeHierarchy
	//
	// Recursively walks the imported NodeData tree and appends nodes
	// to outFlatNodes in pre-order (parent always has a lower index
	// than its children). This guarantees topological order for
	// transform propagation.
	// ================================================================

	void ModelAssetManager::FlattenNodeHierarchy(
		const NodeData& node,
		int32_t parentIndex,
		std::vector<FlatNode>& outFlatNodes)
	{
		int32_t myIndex = (int32_t)outFlatNodes.size();

		FlatNode flat;
		flat.parentIndex = parentIndex;
		flat.localTransform = node.transformation;
		flat.name = node.name;
		flat.meshIndices = node.meshIndices;
		outFlatNodes.push_back(flat);

		for (const NodeData& child : node.children) {
			FlattenNodeHierarchy(child, myIndex, outFlatNodes);
		}
	}

	// ================================================================
	// RegisterModel
	//
	// Takes a loaded Model and:
	// 1. Flattens its node hierarchy
	// 2. Appends all mesh/meshlet/vertex/material data to global pools
	// 3. Creates GpuMeshRecord, GpuNodeRecord, GpuMeshBinding entries
	// 4. Records a GpuModelAssetRecord with offsets into all pools
	// 5. Writes the assetId and pool offsets back to the Model
	//
	// After registration the Model still has its own per-model SSBOs
	// for backward compatibility. Those will be removed in later milestones.
	// ================================================================

	uint32_t ModelAssetManager::RegisterModel(Model* model) {

		uint32_t assetId = (uint32_t)m_assets.size();

		// --- Flatten the node hierarchy ---
		std::vector<FlatNode> flatNodes;
		FlattenNodeHierarchy(model->rootNode, -1, flatNodes);

		// Store flat nodes on the model for CPU-side animation
		model->flatNodes = flatNodes;
		model->assetId = assetId;

		// --- Record pool offsets ---
		GpuModelAssetRecord asset;
		asset.nodeOffset = (uint32_t)m_nodeRecords.size();
		asset.nodeCount = (uint32_t)flatNodes.size();
		asset.meshOffset = (uint32_t)m_meshRecords.size();
		asset.meshCount = (uint32_t)model->meshes.size();
		// Materials were already appended to the global pool by LoadMaterials,
		// so use the pre-recorded base offset from the model.
		asset.materialOffset = model->globalMaterialBase;
		asset.meshBindingOffset = (uint32_t)m_meshBindings.size();

		// --- Vertices: append to global pool ---
		uint32_t globalVertexBase = (uint32_t)m_globalVertices.size();
		m_globalVertices.insert(m_globalVertices.end(),
			model->cpuVertices.begin(), model->cpuVertices.end());

		// --- Meshlets: append to global pool, remapping offsets ---
		uint32_t globalMeshletBase = (uint32_t)m_globalMeshlets.size();
		uint32_t globalMeshletVertexBase = (uint32_t)m_globalMeshletVertexIndices.size();
		uint32_t globalMeshletLocalBase = (uint32_t)m_globalMeshletLocalIndices.size();

		// We need to regenerate meshlets into the global pool.
		// The model already has cpuVertices/cpuIndices and per-mesh offsets.
		// We regenerate meshlets here so that vertex indices point into the
		// global vertex pool.

		for (UINT mi = 0; mi < model->meshes.size(); mi++) {
			Mesh* mesh = model->meshes[mi];

			uint32_t localVertexOffset = model->cpuMeshVertexOffsets[mi];
			uint32_t localIndexOffset = model->cpuMeshIndexOffsets[mi];
			uint32_t indexCount = mesh->mTriangleCount; // actually index count
			uint32_t vertexCount = mesh->mVertexCount;

			// Extract positions for meshoptimizer
			std::vector<float> positions(vertexCount * 3);
			for (uint32_t v = 0; v < vertexCount; v++) {
				const Vertex& vert = model->cpuVertices[localVertexOffset + v];
				positions[v * 3 + 0] = vert.position.x;
				positions[v * 3 + 1] = vert.position.y;
				positions[v * 3 + 2] = vert.position.z;
			}

			const uint32_t* meshIndices = &model->cpuIndices[localIndexOffset];

			size_t maxMeshlets = meshopt_buildMeshletsBound(indexCount, MESHLET_MAX_VERTEX_COUNT, MESHLET_MAX_TRIANGLE_COUNT);
			std::vector<meshopt_Meshlet> meshlets(maxMeshlets);
			std::vector<unsigned int> meshletVertices(maxMeshlets * MESHLET_MAX_VERTEX_COUNT);
			std::vector<unsigned char> meshletTriangles(maxMeshlets * MESHLET_MAX_TRIANGLE_COUNT * 3);

			size_t meshletCount = meshopt_buildMeshlets(
				meshlets.data(), meshletVertices.data(), meshletTriangles.data(),
				meshIndices, indexCount,
				positions.data(), vertexCount, sizeof(float) * 3,
				MESHLET_MAX_VERTEX_COUNT, MESHLET_MAX_TRIANGLE_COUNT, 0.0f
			);

			// Compute mesh bounds from all vertices
			glm::vec3 meshBoundsMin(FLT_MAX);
			glm::vec3 meshBoundsMax(-FLT_MAX);
			for (uint32_t v = 0; v < vertexCount; v++) {
				glm::vec3 pos(positions[v * 3], positions[v * 3 + 1], positions[v * 3 + 2]);
				meshBoundsMin = glm::min(meshBoundsMin, pos);
				meshBoundsMax = glm::max(meshBoundsMax, pos);
			}

			// Create GpuMeshRecord
			GpuMeshRecord meshRecord;
			meshRecord.meshletOffset = (uint32_t)m_globalMeshlets.size();
			meshRecord.meshletCount = (uint32_t)meshletCount;
			meshRecord.materialId = asset.materialOffset + mesh->mMaterial;
			meshRecord.boundsMin = meshBoundsMin;
			meshRecord.boundsMax = meshBoundsMax;
			m_meshRecords.push_back(meshRecord);

			// Also update the Mesh's offsets for backward compatibility
			mesh->meshletOffset = (uint32_t)m_globalMeshlets.size();
			mesh->meshletCount = (uint32_t)meshletCount;
			mesh->baseVertex = globalVertexBase + localVertexOffset;

			// Append meshlets to global pool
			for (size_t i = 0; i < meshletCount; i++) {
				const meshopt_Meshlet& m = meshlets[i];

				GpuMeshlet gpuMeshlet;
				gpuMeshlet.VertexOffset = (uint32_t)m_globalMeshletVertexIndices.size();
				gpuMeshlet.IndicesOffset = (uint32_t)m_globalMeshletLocalIndices.size();
				gpuMeshlet.VertexCount = m.vertex_count;
				gpuMeshlet.TriangleCount = m.triangle_count;

				// Compute meshlet AABB
				GpuMeshletInfo info = {};
				info.Min = glm::vec3(FLT_MAX);
				info.Max = glm::vec3(-FLT_MAX);
				for (uint32_t v = 0; v < m.vertex_count; v++) {
					uint32_t vi = meshletVertices[m.vertex_offset + v];
					glm::vec3 pos(positions[vi * 3], positions[vi * 3 + 1], positions[vi * 3 + 2]);
					info.Min = glm::min(info.Min, pos);
					info.Max = glm::max(info.Max, pos);
				}

				m_globalMeshlets.push_back(gpuMeshlet);
				m_globalMeshletInfos.push_back(info);

				// Vertex indices remapped to global vertex pool
				for (uint32_t v = 0; v < m.vertex_count; v++) {
					m_globalMeshletVertexIndices.push_back(
						globalVertexBase + localVertexOffset + meshletVertices[m.vertex_offset + v]
					);
				}

				// Pack local triangle indices
				uint32_t triangleByteCount = m.triangle_count * 3;
				uint32_t packedCount = (triangleByteCount + 3) / 4;
				for (uint32_t p = 0; p < packedCount; p++) {
					uint32_t packed = 0;
					for (int b = 0; b < 4; b++) {
						uint32_t byteIdx = p * 4 + b;
						if (byteIdx < triangleByteCount) {
							packed |= (uint32_t)meshletTriangles[m.triangle_offset + byteIdx] << (b * 8);
						}
					}
					m_globalMeshletLocalIndices.push_back(packed);
				}
			}
		}

		// --- Materials: we need to read the model's material SSBO back,
		// or re-extract from the GPU material data.
		// Since materials are built and uploaded in LoadMaterials(),
		// and we don't have CPU copies, we'll store them during registration.
		// For now, the model's materialSSBO is still used in the old path.
		// We record the material offset/count so the new path can reference them.
		// The actual material data will be populated by a modified LoadMaterials
		// in Model.cpp that also pushes to the global pool. ---

		// materialCount will be set by Model.cpp when it pushes materials
		asset.materialCount = model->globalMaterialCount;

		// --- Node records and mesh bindings ---
		uint32_t meshBindingCount = 0;
		for (uint32_t ni = 0; ni < flatNodes.size(); ni++) {
			const FlatNode& fn = flatNodes[ni];

			GpuNodeRecord nodeRec;
			nodeRec.parentIndex = fn.parentIndex;
			nodeRec.meshBindingOffset = (uint32_t)m_meshBindings.size();
			nodeRec.meshBindingCount = (uint32_t)fn.meshIndices.size();
			nodeRec.localTransform = fn.localTransform;
			m_nodeRecords.push_back(nodeRec);

			for (uint32_t meshIdx : fn.meshIndices) {
				GpuMeshBinding binding;
				binding.meshId = asset.meshOffset + meshIdx;
				m_meshBindings.push_back(binding);
				meshBindingCount++;
			}
		}
		asset.meshBindingCount = meshBindingCount;

		// Store pool offsets back on the model for backward-compatible rendering
		model->globalVertexBase = globalVertexBase;
		model->globalMeshletBase = globalMeshletBase;
		model->globalNodeBase = asset.nodeOffset;
		model->globalMeshBase = asset.meshOffset;
		model->globalMaterialBase = asset.materialOffset;

		m_assets.push_back(asset);

		return assetId;
	}

	// ================================================================
	// UploadToGPU
	//
	// Creates immutable GPU buffers and uploads all accumulated pool data.
	// Call once after all startup models are loaded.
	// ================================================================

	void ModelAssetManager::UploadToGPU() {

		if (m_uploaded) return;

		auto CreateAndUpload = [](GLuint& buffer, const void* data, size_t size, const char* label) {
			if (size == 0) return;
			glCreateBuffers(1, &buffer);
			glNamedBufferStorage(buffer, size, data, 0); // immutable, no flags needed
			(void)label; // for debug labeling if needed later
		};

		// Vertex pool (binding 8 - reuses existing slot)
		CreateAndUpload(m_vertexPoolSSBO,
			m_globalVertices.data(),
			m_globalVertices.size() * sizeof(Vertex),
			"GlobalVertexPool");

		// Meshlet pool (binding 11)
		CreateAndUpload(m_meshletPoolSSBO,
			m_globalMeshlets.data(),
			m_globalMeshlets.size() * sizeof(GpuMeshlet),
			"GlobalMeshletPool");

		// Meshlet info pool (binding 12)
		CreateAndUpload(m_meshletInfoPoolSSBO,
			m_globalMeshletInfos.data(),
			m_globalMeshletInfos.size() * sizeof(GpuMeshletInfo),
			"GlobalMeshletInfoPool");

		// Meshlet vertex index pool (binding 13)
		CreateAndUpload(m_meshletVertexPoolSSBO,
			m_globalMeshletVertexIndices.data(),
			m_globalMeshletVertexIndices.size() * sizeof(uint32_t),
			"GlobalMeshletVertexPool");

		// Meshlet local index pool (binding 14)
		CreateAndUpload(m_meshletLocalPoolSSBO,
			m_globalMeshletLocalIndices.data(),
			m_globalMeshletLocalIndices.size() * sizeof(uint32_t),
			"GlobalMeshletLocalPool");

		// Material pool (binding 2)
		if (!m_globalMaterials.empty()) {
			CreateAndUpload(m_materialPoolSSBO,
				m_globalMaterials.data(),
				m_globalMaterials.size() * sizeof(GPUMaterial),
				"GlobalMaterialPool");
		}

		// Mesh record pool (binding 15)
		CreateAndUpload(m_meshRecordPoolSSBO,
			m_meshRecords.data(),
			m_meshRecords.size() * sizeof(GpuMeshRecord),
			"GlobalMeshRecordPool");

		// Node record pool (binding 16)
		CreateAndUpload(m_nodeRecordPoolSSBO,
			m_nodeRecords.data(),
			m_nodeRecords.size() * sizeof(GpuNodeRecord),
			"GlobalNodeRecordPool");

		// Mesh binding pool (binding 17)
		CreateAndUpload(m_meshBindingPoolSSBO,
			m_meshBindings.data(),
			m_meshBindings.size() * sizeof(GpuMeshBinding),
			"GlobalMeshBindingPool");

		// Asset pool (binding 18)
		CreateAndUpload(m_assetPoolSSBO,
			m_assets.data(),
			m_assets.size() * sizeof(GpuModelAssetRecord),
			"GlobalAssetPool");

		m_uploaded = true;

#ifdef _MYDEBUG
		LOG_INFO(L"ModelAssetManager: Uploaded global pools to GPU");
		LOG_INFO(L"  Assets: %u", (uint32_t)m_assets.size());
		LOG_INFO(L"  Vertices: %u", (uint32_t)m_globalVertices.size());
		LOG_INFO(L"  Meshlets: %u", (uint32_t)m_globalMeshlets.size());
		LOG_INFO(L"  Materials: %u", (uint32_t)m_globalMaterials.size());
		LOG_INFO(L"  Mesh records: %u", (uint32_t)m_meshRecords.size());
		LOG_INFO(L"  Node records: %u", (uint32_t)m_nodeRecords.size());
		LOG_INFO(L"  Mesh bindings: %u", (uint32_t)m_meshBindings.size());
#endif
	}

	// ================================================================
	// BindGlobalBuffers
	//
	// Binds all global immutable SSBOs. Call once per frame before
	// any mesh-shader draw calls.
	// ================================================================

	void ModelAssetManager::BindGlobalBuffers() const {

		if (!m_uploaded) return;

		// Reused existing binding slots (backward compatible)
		if (m_materialPoolSSBO)
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_materialPoolSSBO);
		if (m_vertexPoolSSBO)
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, m_vertexPoolSSBO);
		if (m_meshletPoolSSBO)
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, m_meshletPoolSSBO);
		if (m_meshletInfoPoolSSBO)
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 12, m_meshletInfoPoolSSBO);
		if (m_meshletVertexPoolSSBO)
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 13, m_meshletVertexPoolSSBO);
		if (m_meshletLocalPoolSSBO)
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 14, m_meshletLocalPoolSSBO);

		// New binding slots
		if (m_meshRecordPoolSSBO)
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, GlobalBindings::MeshRecordPool, m_meshRecordPoolSSBO);
		if (m_nodeRecordPoolSSBO)
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, GlobalBindings::NodeRecordPool, m_nodeRecordPoolSSBO);
		if (m_meshBindingPoolSSBO)
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, GlobalBindings::MeshBindingPool, m_meshBindingPoolSSBO);
		if (m_assetPoolSSBO)
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, GlobalBindings::AssetPool, m_assetPoolSSBO);
	}

} // namespace AMC
