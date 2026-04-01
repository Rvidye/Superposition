#include <SkinningSystem.h>

namespace AMC {

	SkinningSystem& SkinningSystem::Get() {
		static SkinningSystem instance;
		return instance;
	}

	void SkinningSystem::RegisterSkeletalInstance(uint32_t instanceId, Model* model) {
		if (!model || !model->haveAnimation || model->animType != SKELETALANIM) return;
		if (m_instanceToRecord.count(instanceId)) return;

		SkinningRecord rec;
		rec.instanceId = instanceId;
		rec.model = model;
		rec.boneOffset = m_nextBoneSlot;
		rec.boneCount = MAX_BONE_COUNT;
		m_nextBoneSlot += MAX_BONE_COUNT;

		// Count total vertices across all meshes
		uint32_t totalVerts = 0;
		for (auto* mesh : model->meshes) {
			totalVerts += mesh->mVertexCount;
		}
		rec.skinnedVertexOffset = m_nextSkinnedVertexSlot;
		rec.skinnedVertexCount = totalVerts;
		m_nextSkinnedVertexSlot += totalVerts;

		uint32_t recIdx = (uint32_t)m_records.size();
		m_instanceToRecord[instanceId] = recIdx;
		m_records.push_back(rec);

		// Grow bone palettes
		m_currentBones.resize(m_nextBoneSlot, glm::mat4(1.0f));
		m_prevBones.resize(m_nextBoneSlot, glm::mat4(1.0f));
	}

	bool SkinningSystem::HasSkinning(uint32_t instanceId) const {
		return m_instanceToRecord.count(instanceId) > 0;
	}

	uint32_t SkinningSystem::GetSkinnedVertexOffset(uint32_t instanceId) const {
		auto it = m_instanceToRecord.find(instanceId);
		if (it == m_instanceToRecord.end()) return 0;
		return m_records[it->second].skinnedVertexOffset;
	}

	// ================================================================
	// EvaluateBones - reuses the existing CalculateBoneTransform logic
	// from Model.cpp but writes to the global bone palette.
	// ================================================================

	static Bone* FindBone(SkeletonAnimator* a, const std::string& name) {
		for (UINT i = 0; i < a->bones.size(); i++) {
			if (a->bones[i].name == name) return &a->bones[i];
		}
		return nullptr;
	}

	static float GetScaleFactorSkin(float last, float next, float t) {
		float mid = t - last;
		float diff = next - last;
		return (diff > 0.0f) ? mid / diff : 0.0f;
	}

	static void EvalBoneRecursive(
		Model* model, SkeletonAnimator* a,
		const AssimpNodeData* node, const glm::mat4& parentTransform,
		glm::mat4* outBones)
	{
		std::string nodeName = node->name;
		glm::mat4 nodeTransform = node->transformation;

		Bone* bone = FindBone(a, nodeName);
		if (bone) {
			glm::mat4 T(1.0f), R(1.0f), S(1.0f);
			float t = a->currentTime;

			// Translation
			if (bone->positions.size() == 1) {
				T = glm::translate(glm::mat4(1.0f), bone->positions[0].position);
			} else if (!bone->positions.empty()) {
				int p0 = 0;
				for (; p0 < (int)bone->positions.size() - 1; ++p0)
					if (t < bone->positions[p0 + 1].time) break;
				int p1 = p0 + 1;
				if (p1 < (int)bone->positions.size()) {
					float sf = GetScaleFactorSkin(bone->positions[p0].time, bone->positions[p1].time, t);
					T = glm::translate(glm::mat4(1.0f), glm::mix(bone->positions[p0].position, bone->positions[p1].position, sf));
				} else {
					T = glm::translate(glm::mat4(1.0f), bone->positions[p0].position);
				}
			}

			// Rotation
			if (bone->rotations.size() == 1) {
				R = glm::mat4_cast(glm::normalize(bone->rotations[0].orientation));
			} else if (!bone->rotations.empty()) {
				int p0 = 0;
				for (; p0 < (int)bone->rotations.size() - 1; ++p0)
					if (t < bone->rotations[p0 + 1].time) break;
				int p1 = p0 + 1;
				if (p1 < (int)bone->rotations.size()) {
					float sf = GetScaleFactorSkin(bone->rotations[p0].time, bone->rotations[p1].time, t);
					R = glm::mat4_cast(glm::normalize(glm::slerp(bone->rotations[p0].orientation, bone->rotations[p1].orientation, sf)));
				} else {
					R = glm::mat4_cast(glm::normalize(bone->rotations[p0].orientation));
				}
			}

			// Scale
			if (bone->scales.size() == 1) {
				S = glm::scale(glm::mat4(1.0f), bone->scales[0].scale);
			} else if (!bone->scales.empty()) {
				int p0 = 0;
				for (; p0 < (int)bone->scales.size() - 1; ++p0)
					if (t < bone->scales[p0 + 1].time) break;
				int p1 = p0 + 1;
				if (p1 < (int)bone->scales.size()) {
					float sf = GetScaleFactorSkin(bone->scales[p0].time, bone->scales[p1].time, t);
					S = glm::scale(glm::mat4(1.0f), glm::mix(bone->scales[p0].scale, bone->scales[p1].scale, sf));
				} else {
					S = glm::scale(glm::mat4(1.0f), bone->scales[p0].scale);
				}
			}

			nodeTransform = T * R * S;
		}

		glm::mat4 globalTransform = parentTransform * nodeTransform;

		auto& boneMap = model->BoneInfoMap;
		auto it = boneMap.find(nodeName);
		if (it != boneMap.end()) {
			int idx = it->second.id;
			outBones[idx] = globalTransform * it->second.offset;
		}

		for (size_t i = 0; i < node->children.size(); i++) {
			EvalBoneRecursive(model, a, &node->children[i], globalTransform, outBones);
		}
	}

	void SkinningSystem::EvaluateBones(SkinningRecord& rec, float deltaTime) {
		Model* model = rec.model;
		int animIdx = model->CurrentAnimation;
		if (animIdx < 0 || animIdx >= (int)model->skeletonAnimator.size()) return;

		SkeletonAnimator& anim = model->skeletonAnimator[animIdx];

		// Advance time
		anim.currentTime += anim.ticksPerSecond * deltaTime;
		anim.currentTime = fmod(anim.currentTime, anim.duration);

		// Write to global palette
		glm::mat4* bones = &m_currentBones[rec.boneOffset];
		for (uint32_t i = 0; i < rec.boneCount; i++) bones[i] = glm::mat4(1.0f);

		EvalBoneRecursive(model, &anim, &anim.rootNode, glm::mat4(1.0f), bones);
	}

	// ================================================================
	// Update - per-frame skeletal animation + GPU skinning
	// ================================================================

	void SkinningSystem::Update(float deltaTime) {
		if (m_records.empty()) return;

		// Swap current → previous bones
		m_prevBones = m_currentBones;

		// Evaluate bones for each skeletal instance
		for (auto& rec : m_records) {
			auto& instMgr = SceneInstanceManager::Get();
			const auto& inst = instMgr.GetInstance(rec.instanceId);
			if (!(inst.flags & InstanceFlags::Visible)) continue;

			EvaluateBones(rec, deltaTime);
		}

		// Create GPU buffers
		if (!m_gpuCreated && !m_currentBones.empty()) {
			glCreateBuffers(1, &m_currentBoneSSBO);
			glCreateBuffers(1, &m_prevBoneSSBO);
			glCreateBuffers(1, &m_currentSkinnedVertexSSBO);
			glCreateBuffers(1, &m_prevSkinnedVertexSSBO);
			m_gpuCreated = true;
		}

		if (!m_gpuCreated) return;

		// Upload bone palettes
		size_t boneBytes = m_currentBones.size() * sizeof(glm::mat4);
		glNamedBufferData(m_currentBoneSSBO, boneBytes, m_currentBones.data(), GL_DYNAMIC_DRAW);
		glNamedBufferData(m_prevBoneSSBO, boneBytes, m_prevBones.data(), GL_DYNAMIC_DRAW);

		// Allocate skinned vertex buffers if needed
		if (m_nextSkinnedVertexSlot > 0) {
			size_t vertBytes = m_nextSkinnedVertexSlot * sizeof(Vertex);
			// Swap previous ← current before overwriting
			std::swap(m_currentSkinnedVertexSSBO, m_prevSkinnedVertexSSBO);
			glNamedBufferData(m_currentSkinnedVertexSSBO, vertBytes, nullptr, GL_DYNAMIC_DRAW);
		}

		// Dispatch GPU compute skinning for each record
		// For now, we still use the existing per-model ComputeSkin path
		// via Model::update(). The global path will be wired when mesh
		// shaders support skeletal models.
		//
		// TODO: Dispatch global skinning compute here once we have
		// a global skinning compute shader that reads from the global
		// bone palette and writes to the global skinned vertex pool.
	}

	void SkinningSystem::BindBuffers() const {
		if (!m_gpuCreated) return;
		// Bone palettes at binding 24/25 (new slots for global bones)
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 24, m_currentBoneSSBO);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 25, m_prevBoneSSBO);
		if (m_currentSkinnedVertexSSBO)
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 26, m_currentSkinnedVertexSSBO);
		if (m_prevSkinnedVertexSSBO)
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 27, m_prevSkinnedVertexSSBO);
	}

} // namespace AMC
