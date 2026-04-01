#include <SceneInstanceManager.h>
#include <ModelAssetManager.h>

namespace AMC {

	SceneInstanceManager& SceneInstanceManager::Get() {
		static SceneInstanceManager instance;
		return instance;
	}

	// ================================================================
	// CreateInstance
	//
	// Allocates a contiguous range in the transform palette for this
	// instance's nodes. Initializes with rest-pose transforms.
	// ================================================================

	uint32_t SceneInstanceManager::CreateInstance(
		Model* model,
		const glm::mat4& worldMatrix,
		const std::string& sceneName)
	{
		if (!model || model->assetId < 0) return UINT32_MAX;

		uint32_t instanceId = (uint32_t)m_instances.size();
		uint32_t nodeCount = (uint32_t)model->flatNodes.size();
		uint32_t transformBase = m_nextTransformSlot;
		m_nextTransformSlot += nodeCount;

		// Grow palette arrays
		m_currentTransforms.resize(m_nextTransformSlot, glm::mat4(1.0f));
		m_prevTransforms.resize(m_nextTransformSlot, glm::mat4(1.0f));
		m_localTransforms.resize(m_nextTransformSlot, glm::mat4(1.0f));

		// Initialize local transforms from rest pose
		for (uint32_t i = 0; i < nodeCount; i++) {
			m_localTransforms[transformBase + i] = model->flatNodes[i].localTransform;
		}

		// Build instance record
		InstanceRecord inst;
		inst.assetId = (uint32_t)model->assetId;
		inst.transformBase = transformBase;
		inst.nodeCount = nodeCount;
		inst.flags = InstanceFlags::Visible;
		inst.model = model;
		inst.worldMatrix = worldMatrix;
		inst.prevWorldMatrix = worldMatrix;
		inst.activeAnimation = model->CurrentAnimation;
		inst.sceneName = sceneName;

		if (model->haveAnimation) {
			inst.flags |= InstanceFlags::Animated;
			if (model->animType == SKELETALANIM) {
				inst.flags |= InstanceFlags::Skeletal;
			}
		}

		m_instances.push_back(inst);

		// Immediately propagate rest-pose transforms so the first frame is correct
		PropagateTransforms(m_instances.back());

		// Copy current to previous so first frame has no velocity
		for (uint32_t i = 0; i < nodeCount; i++) {
			m_prevTransforms[transformBase + i] = m_currentTransforms[transformBase + i];
		}

		return instanceId;
	}

	void SceneInstanceManager::RemoveInstance(uint32_t instanceId) {
		if (instanceId < m_instances.size()) {
			m_instances[instanceId].flags &= ~InstanceFlags::Visible;
		}
	}

	void SceneInstanceManager::SetVisible(uint32_t instanceId, bool visible) {
		if (instanceId < m_instances.size()) {
			if (visible)
				m_instances[instanceId].flags |= InstanceFlags::Visible;
			else
				m_instances[instanceId].flags &= ~InstanceFlags::Visible;
		}
	}

	void SceneInstanceManager::SetWorldMatrix(uint32_t instanceId, const glm::mat4& matrix) {
		if (instanceId < m_instances.size()) {
			m_instances[instanceId].worldMatrix = matrix;
		}
	}

	const glm::mat4& SceneInstanceManager::GetCurrentTransform(uint32_t instanceId, uint32_t nodeIndex) const {
		return m_currentTransforms[m_instances[instanceId].transformBase + nodeIndex];
	}

	const glm::mat4& SceneInstanceManager::GetPrevTransform(uint32_t instanceId, uint32_t nodeIndex) const {
		return m_prevTransforms[m_instances[instanceId].transformBase + nodeIndex];
	}

	// ================================================================
	// BeginFrame
	//
	// 1. Copy current palette → previous palette
	// 2. For each animated instance, evaluate node animation keyframes
	//    into local transforms
	// 3. Propagate world transforms through flat hierarchy
	//
	// For skeletal instances: the old Model::update() path still handles
	// bone transforms and ComputeSkin. We only handle KEYFRAMEANIM here.
	// ================================================================

	void SceneInstanceManager::BeginFrame(float deltaTime) {

		// Step 1: current → previous
		if (!m_currentTransforms.empty()) {
			memcpy(m_prevTransforms.data(), m_currentTransforms.data(),
				m_currentTransforms.size() * sizeof(glm::mat4));
		}

		// Step 2 & 3: evaluate animation and propagate
		for (auto& inst : m_instances) {
			if (!(inst.flags & InstanceFlags::Visible)) continue;

			// Sync world matrix from scene (the scene may have updated it)
			inst.prevWorldMatrix = inst.worldMatrix;

			if ((inst.flags & InstanceFlags::Animated) &&
				!(inst.flags & InstanceFlags::Skeletal))
			{
				EvaluateNodeAnimation(inst, deltaTime);
			}

			PropagateTransforms(inst);
		}
	}

	// ================================================================
	// EvaluateNodeAnimation
	//
	// Evaluates GLTF node animation keyframes and writes interpolated
	// local transforms into m_localTransforms. This replaces the
	// recursive CalculateNodeTransform for the flat-palette path.
	//
	// Uses the same interpolation logic as Model.cpp but writes to
	// the flat array instead of NodeData.globalTransform.
	// ================================================================

	static float GetScaleFactor(float lastTime, float nextTime, float animTime) {
		float midWay = animTime - lastTime;
		float framesDiff = nextTime - lastTime;
		return (framesDiff > 0.0f) ? midWay / framesDiff : 0.0f;
	}

	void SceneInstanceManager::EvaluateNodeAnimation(InstanceRecord& inst, float deltaTime) {

		Model* model = inst.model;
		if (!model || model->animType != KEYFRAMEANIM) return;
		if (inst.activeAnimation < 0 || inst.activeAnimation >= (int32_t)model->nodeAnimator.size()) return;

		NodeAnimator& animator = model->nodeAnimator[inst.activeAnimation];

		// Advance animation time
		animator.currentTime += animator.ticksPerSecond * deltaTime;
		animator.currentTime = fmod(animator.currentTime, animator.duration);
		float currentTime = animator.currentTime;

		// For each flat node, check if it has an animation channel
		for (uint32_t ni = 0; ni < inst.nodeCount; ni++) {
			const FlatNode& fn = model->flatNodes[ni];

			auto it = animator.nodeAnimations.find(fn.name);
			if (it == animator.nodeAnimations.end()) {
				// No animation for this node — keep rest-pose local transform
				m_localTransforms[inst.transformBase + ni] = fn.localTransform;
				continue;
			}

			const NodeAnimation& nodeAnim = it->second;
			glm::mat4 translationMat(1.0f);
			glm::mat4 rotationMat(1.0f);
			glm::mat4 scalingMat(1.0f);

			// --- Translation ---
			if (nodeAnim.positions.size() == 1) {
				translationMat = glm::translate(glm::mat4(1.0f), nodeAnim.positions[0].position);
			}
			else if (!nodeAnim.positions.empty()) {
				int p0 = 0;
				for (p0 = 0; p0 < (int)nodeAnim.positions.size() - 1; ++p0) {
					if (currentTime < nodeAnim.positions[p0 + 1].time) break;
				}
				if (p0 >= (int)nodeAnim.positions.size() - 1) {
					translationMat = glm::translate(glm::mat4(1.0f), nodeAnim.positions[p0].position);
				}
				else {
					int p1 = p0 + 1;
					float sf = GetScaleFactor(nodeAnim.positions[p0].time, nodeAnim.positions[p1].time, currentTime);
					glm::vec3 pos = glm::mix(nodeAnim.positions[p0].position, nodeAnim.positions[p1].position, sf);
					translationMat = glm::translate(glm::mat4(1.0f), pos);
				}
			}

			// --- Rotation ---
			if (nodeAnim.rotations.size() == 1) {
				rotationMat = glm::mat4_cast(glm::normalize(nodeAnim.rotations[0].orientation));
			}
			else if (!nodeAnim.rotations.empty()) {
				int p0 = 0;
				for (p0 = 0; p0 < (int)nodeAnim.rotations.size() - 1; ++p0) {
					if (currentTime < nodeAnim.rotations[p0 + 1].time) break;
				}
				if (p0 >= (int)nodeAnim.rotations.size() - 1) {
					rotationMat = glm::mat4_cast(glm::normalize(nodeAnim.rotations[p0].orientation));
				}
				else {
					int p1 = p0 + 1;
					float sf = GetScaleFactor(nodeAnim.rotations[p0].time, nodeAnim.rotations[p1].time, currentTime);
					glm::quat q = glm::normalize(glm::slerp(
						nodeAnim.rotations[p0].orientation,
						nodeAnim.rotations[p1].orientation, sf));
					rotationMat = glm::mat4_cast(q);
				}
			}

			// --- Scale ---
			if (nodeAnim.scales.size() == 1) {
				scalingMat = glm::scale(glm::mat4(1.0f), nodeAnim.scales[0].scale);
			}
			else if (!nodeAnim.scales.empty()) {
				int p0 = 0;
				for (p0 = 0; p0 < (int)nodeAnim.scales.size() - 1; ++p0) {
					if (currentTime < nodeAnim.scales[p0 + 1].time) break;
				}
				if (p0 >= (int)nodeAnim.scales.size() - 1) {
					scalingMat = glm::scale(glm::mat4(1.0f), nodeAnim.scales[p0].scale);
				}
				else {
					int p1 = p0 + 1;
					float sf = GetScaleFactor(nodeAnim.scales[p0].time, nodeAnim.scales[p1].time, currentTime);
					glm::vec3 s = glm::mix(nodeAnim.scales[p0].scale, nodeAnim.scales[p1].scale, sf);
					scalingMat = glm::scale(glm::mat4(1.0f), s);
				}
			}

			m_localTransforms[inst.transformBase + ni] = translationMat * rotationMat * scalingMat;
		}
	}

	// ================================================================
	// PropagateTransforms
	//
	// Walks the flat node array in topological order (index 0..N-1).
	// Because parent always has lower index than children, a single
	// forward pass computes all world transforms.
	//
	// currentTransform[base + i] = worldMatrix * parent_world * local[i]
	//
	// For root nodes (parentIndex == -1), parent_world = identity.
	// The instance's worldMatrix is multiplied at the root level.
	// ================================================================

	void SceneInstanceManager::PropagateTransforms(InstanceRecord& inst) {

		uint32_t base = inst.transformBase;

		for (uint32_t i = 0; i < inst.nodeCount; i++) {
			int32_t parentIdx = inst.model->flatNodes[i].parentIndex;

			glm::mat4 parentWorld;
			if (parentIdx < 0) {
				// Root node: multiply by instance world matrix
				parentWorld = inst.worldMatrix;
			}
			else {
				parentWorld = m_currentTransforms[base + parentIdx];
			}

			m_currentTransforms[base + i] = parentWorld * m_localTransforms[base + i];
		}
	}

	// ================================================================
	// UploadToGPU
	//
	// Uploads current and previous transform palettes and instance
	// data to GPU SSBOs. Uses glNamedBufferData for simplicity
	// (dynamic buffers, re-uploaded each frame).
	// ================================================================

	void SceneInstanceManager::UploadToGPU() {

		if (m_currentTransforms.empty()) return;

		size_t transformBytes = m_currentTransforms.size() * sizeof(glm::mat4);

		// Create buffers on first use
		if (!m_gpuBuffersCreated) {
			glCreateBuffers(1, &m_currentTransformSSBO);
			glCreateBuffers(1, &m_prevTransformSSBO);
			glCreateBuffers(1, &m_instanceSSBO);
			m_gpuBuffersCreated = true;
		}

		// Upload transforms
		glNamedBufferData(m_currentTransformSSBO, transformBytes,
			m_currentTransforms.data(), GL_DYNAMIC_DRAW);
		glNamedBufferData(m_prevTransformSSBO, transformBytes,
			m_prevTransforms.data(), GL_DYNAMIC_DRAW);

		// Build and upload instance array
		m_gpuInstances.resize(m_instances.size());
		for (uint32_t i = 0; i < m_instances.size(); i++) {
			m_gpuInstances[i].assetId = m_instances[i].assetId;
			m_gpuInstances[i].transformBase = m_instances[i].transformBase;
			m_gpuInstances[i].nodeCount = m_instances[i].nodeCount;
			m_gpuInstances[i].flags = m_instances[i].flags;
		}

		glNamedBufferData(m_instanceSSBO,
			m_gpuInstances.size() * sizeof(GpuModelInstance),
			m_gpuInstances.data(), GL_DYNAMIC_DRAW);
	}

	// ================================================================
	// BindBuffers
	// ================================================================

	void SceneInstanceManager::BindBuffers() const {
		if (!m_gpuBuffersCreated) return;

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER,
			GlobalBindings::TransformPalette, m_currentTransformSSBO);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER,
			GlobalBindings::PrevTransformPalette, m_prevTransformSSBO);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER,
			GlobalBindings::InstancePool, m_instanceSSBO);
	}

} // namespace AMC
