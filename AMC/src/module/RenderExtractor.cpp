#include <RenderExtractor.h>

namespace AMC {

	RenderExtractor& RenderExtractor::Get() {
		static RenderExtractor instance;
		return instance;
	}

	// ================================================================
	// Extract
	//
	// Iterates all visible, non-skeletal instances. For each node
	// that has meshes, creates a GpuRenderItem and a corresponding
	// GpuMeshTaskCommand (one indirect draw per mesh).
	// ================================================================

	void RenderExtractor::Extract() {
		m_renderItems.clear();
		m_commands.clear();

		auto& assetMgr = ModelAssetManager::Get();
		auto& instMgr = SceneInstanceManager::Get();

		const auto& meshRecords = assetMgr.GetMeshRecords();
		const auto& nodeRecords = assetMgr.GetNodeRecords();
		const auto& meshBindings = assetMgr.GetMeshBindings();

		uint32_t instanceCount = instMgr.GetInstanceCount();
		for (uint32_t instId = 0; instId < instanceCount; instId++) {
			const InstanceRecord& inst = instMgr.GetInstance(instId);

			// Skip invisible and skeletal instances
			if (!(inst.flags & InstanceFlags::Visible)) continue;
			if (inst.flags & InstanceFlags::Skeletal) continue;

			// Skip models without meshlet data
			if (!inst.model || !inst.model->hasMeshletData) continue;

			const GpuModelAssetRecord& asset = assetMgr.GetAsset(inst.assetId);

			// Iterate nodes for this asset
			for (uint32_t ni = 0; ni < asset.nodeCount; ni++) {
				const GpuNodeRecord& nodeRec = nodeRecords[asset.nodeOffset + ni];
				if (nodeRec.meshBindingCount == 0) continue;

				uint32_t transformIndex = inst.transformBase + ni;
				const glm::mat4& worldMat = instMgr.GetCurrentTransform(instId, ni);

				for (uint32_t bi = 0; bi < nodeRec.meshBindingCount; bi++) {
					uint32_t meshId = meshBindings[nodeRec.meshBindingOffset + bi].meshId;
					const GpuMeshRecord& meshRec = meshRecords[meshId];
					if (meshRec.meshletCount == 0) continue;

					// Transform local AABB to world-space AABB (conservative)
					glm::vec3 corners[8] = {
						{meshRec.boundsMin.x, meshRec.boundsMin.y, meshRec.boundsMin.z},
						{meshRec.boundsMax.x, meshRec.boundsMin.y, meshRec.boundsMin.z},
						{meshRec.boundsMin.x, meshRec.boundsMax.y, meshRec.boundsMin.z},
						{meshRec.boundsMax.x, meshRec.boundsMax.y, meshRec.boundsMin.z},
						{meshRec.boundsMin.x, meshRec.boundsMin.y, meshRec.boundsMax.z},
						{meshRec.boundsMax.x, meshRec.boundsMin.y, meshRec.boundsMax.z},
						{meshRec.boundsMin.x, meshRec.boundsMax.y, meshRec.boundsMax.z},
						{meshRec.boundsMax.x, meshRec.boundsMax.y, meshRec.boundsMax.z},
					};

					glm::vec3 worldMin(FLT_MAX);
					glm::vec3 worldMax(-FLT_MAX);
					for (int c = 0; c < 8; c++) {
						glm::vec3 wp = glm::vec3(worldMat * glm::vec4(corners[c], 1.0f));
						worldMin = glm::min(worldMin, wp);
						worldMax = glm::max(worldMax, wp);
					}

					GpuRenderItem item;
					item.meshId = meshId;
					item.transformIndex = transformIndex;
					item.materialId = meshRec.materialId;
					item.instanceId = instId;
					item.worldBoundsMin = worldMin;
					item.worldBoundsMax = worldMax;

					uint32_t renderItemId = (uint32_t)m_renderItems.size();
					m_renderItems.push_back(item);

					GpuMeshTaskCommand cmd;
					cmd.count = (meshRec.meshletCount + 31) / 32;
					cmd.first = 0;
					cmd.renderItemId = renderItemId;
					cmd._pad0 = 0;
					m_commands.push_back(cmd);
				}
			}
		}
	}

	// ================================================================
	// UploadToGPU
	// ================================================================

	void RenderExtractor::UploadToGPU() {
		if (!m_gpuCreated) {
			glCreateBuffers(1, &m_renderItemSSBO);
			glCreateBuffers(1, &m_commandSSBO);
			glCreateBuffers(1, &m_countBuffer);
			m_gpuCreated = true;
		}

		if (!m_renderItems.empty()) {
			glNamedBufferData(m_renderItemSSBO,
				m_renderItems.size() * sizeof(GpuRenderItem),
				m_renderItems.data(), GL_DYNAMIC_DRAW);
		}

		if (!m_commands.empty()) {
			glNamedBufferData(m_commandSSBO,
				m_commands.size() * sizeof(GpuMeshTaskCommand),
				m_commands.data(), GL_DYNAMIC_DRAW);
		}

		// Upload draw count to parameter buffer
		uint32_t drawCount = (uint32_t)m_commands.size();
		glNamedBufferData(m_countBuffer, sizeof(uint32_t), &drawCount, GL_DYNAMIC_DRAW);
	}

	// ================================================================
	// BindBuffers
	// ================================================================

	void RenderExtractor::BindBuffers() const {
		if (m_renderItemSSBO)
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, GlobalBindings::RenderItemPool, m_renderItemSSBO);
		if (m_commandSSBO)
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, GlobalBindings::MeshTaskCmdPool, m_commandSSBO);
	}

	bool RenderExtractor::ExtractIfDirty(uint64_t geometryVersion) {
		if (geometryVersion == m_lastGeometryVersion && !m_renderItems.empty()) {
			return false; // skip - nothing changed
		}
		Extract();
		UploadToGPU();
		m_lastGeometryVersion = geometryVersion;
		return true;
	}

} // namespace AMC
