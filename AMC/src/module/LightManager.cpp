#include<common.h>
#include<LightManager.h>
#include<Camera.h>
#include<glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include<glm/gtx/quaternion.hpp>

namespace AMC {

	ShaderProgram* LightManager::m_program = nullptr;
	Model* LightManager::directional = nullptr;
	Model* LightManager::spot = nullptr;
	Model* LightManager::point = nullptr;

	Light::Light() {
		gpuLight.active = false;
		gpuLight.type = LIGHT_TYPE_POINT;
		gpuLight.color = glm::vec3(1.0f);
		gpuLight.intensity = 1.0f;
		gpuLight.position = glm::vec3(0.0f);
		gpuLight.direction = glm::vec3(0.0f, -1.0f, 0.0f);
		gpuLight.range = 1.0f;
		gpuLight.spotAngle = glm::radians(45.0f);
		gpuLight.spotExponent = 1.0f;
		gpuLight.shadows = false;
		gpuLight.shadowMapIndex = -1;
	}

	AMC::Light::~Light()
	{
	}

	bool Light::HasShadow() const {
		return gpuLight.shadowMapIndex >= 0;
	}

	void Light::ConnectShadow(int shadowIndex) {
		gpuLight.shadowMapIndex = shadowIndex;
	}

	void Light::DisconnectShadow() {
		gpuLight.shadowMapIndex = -1;
	}

	LightManager::LightManager()
	{
		lights.reserve(MAX_LIGHTS);
		shadowManager = new ShadowManager();
		glCreateBuffers(1, &uboLights);
		glNamedBufferData(uboLights, sizeof(GpuLight) * MAX_LIGHTS + sizeof(int), nullptr, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, 1, uboLights);

		if (!m_program) {
			m_program = new ShaderProgram({ RESOURCE_PATH("shaders\\model\\model.vert"), RESOURCE_PATH("shaders\\color\\color.frag") });
		}

		if (!directional) {
			directional = new AMC::Model(RESOURCE_PATH("models\\debug\\arrow.obj"), aiProcessPreset_TargetRealtime_Quality | aiProcess_FlipUVs | aiProcess_GenBoundingBoxes);
		}

		if (!spot) {
			spot = new AMC::Model(RESOURCE_PATH("models\\debug\\cone.obj"), aiProcessPreset_TargetRealtime_Quality | aiProcess_FlipUVs | aiProcess_GenBoundingBoxes);
		}

		if (!point) {
			point = new AMC::Model(RESOURCE_PATH("models\\debug\\point.obj"), aiProcessPreset_TargetRealtime_Quality | aiProcess_FlipUVs | aiProcess_GenBoundingBoxes);
		}
	}

	LightManager::~LightManager()
	{
		glDeleteBuffers(1, &uboLights);
		if (shadowManager)
			delete shadowManager;
	}

	bool LightManager::AddLight(Light& l) {
		if (lights.size() >= MAX_LIGHTS) {
			LOG_ERROR(L"Max Lights Limit Reached\n");
			return false;
		}
		lights.push_back(l);
		// If the light has shadows enabled, create a shadow
		if (l.gpuLight.shadows) {
			if (shadowManager->AddShadow(static_cast<int>(lights.size() - 1), l)) {
				lights.back().ConnectShadow(static_cast<int>(shadowManager->shadows.size() - 1));
			}
		}

		UpdateUBO();
		return true;
	}

	void LightManager::DeleteLight(int index)
	{
		if (index < 0 || index >= static_cast<int>(lights.size())) {
			LOG_ERROR(L"Lights %d index does not exist\n", index);
			return;
		}

		// If the light has an associated shadow, remove it
		if (lights[index].HasShadow()) {
			shadowManager->DeleteShadow(lights[index].gpuLight.shadowMapIndex);
		}

		// Swap with the last light to maintain a contiguous array
		lights[index] = lights.back();
		lights.pop_back();

		//// If the swapped light had a shadow, update its shadow index
		//if (lights[index].HasShadow()) {
		//	shadowManager->UpdateShadowLightIndex(lights[index].gpuLight.shadowMapIndex, index);
		//}

		UpdateUBO();
	}

	Light* LightManager::GetLight(int index)
	{
		if (index < 0 || index >= static_cast<int>(lights.size())) {
			return nullptr;
		}
		return &lights[index];
	}

	ShadowManager* LightManager::GetShadowManager()
	{
		return shadowManager;
	}

	//void LightManager::removeLight(int index) {
	//	if (index >= 0 && index < lights.size()) {
	//		if (lights[index].shadows) {
	//			shadowManager->removeShadowMapForLight(lights[index]);
	//		}
	//		lights.erase(lights.begin() + index);
	//		this->updateUBO();
	//	}
	//}

	// IDK but if user want to access that specific light element they can use it, but they'll have to call update on their own
	// haven't though this through, might bite me later.
	//Light* LightManager::getLight(int index) {
	//	if (index < 0 || index >= (int)lights.size())
	//		return nullptr;
	//	return &lights[index]; // fuck it
	//}

	//std::vector<Light*> LightManager::getShadowCastingLights() {
	//	std::vector<Light*> result;
	//	for (auto& l : lights) {
	//		if (l.shadows) result.push_back(&l);
	//	}
	//	return result;
	//}

	//ShadowManager* LightManager::getShadowMapManager() {
	//	return shadowManager;
	//}

	//void LightManager::toggleLightShadow(Light& light, bool enable) {
	//	if (enable && !light.shadows) {
	//		shadowManager->createShadowMapForLight(light);
	//		//light.shadows = true;
	//		updateUBO();
	//	}
	//	else if (!enable && light.shadows) {
	//		shadowManager->removeShadowMapForLight(light);
	//		//light.shadows = false;
	//		updateUBO();
	//	}
	//}


	//void LightManager::updateUBO() {
	//	// there has to be a way to only update specific light instead of all lights but for now fuck it.
	//	LightBlock lb = {};
	//	int count = 0; // Instead of just pushing all lights, we only push the active ones.
	//	for (int i = 0; i < (int)lights.size(); ++i) {
	//		const Light& src = lights[i];
	//		//if (!src.active) continue; // skip inactive lights

	//		GpuLight& dst = lb.u_Lights[i];
	//		dst.position = src.position;
	//		dst.intensity = src.intensity;
	//		dst.direction = src.direction;
	//		dst.range = src.range;
	//		dst.color = src.color;
	//		dst.spotAngle = src.spotAngle;
	//		dst.spotExponent = src.spotExponent;
	//		dst.type = src.type;
	//		dst.shadows = src.shadows ? 1 : 0;
	//		dst.shadowMapIndex = src.shadowIndex;
	//		dst.active = src.active ? 1 : 0;
	//		dst.pad = 0.0f;
	//		count++;
	//	}
	//	lb.u_LightCount = count;
	//	glNamedBufferSubData(uboLights, 0, sizeof(LightBlock), &lb);
	//}

	void LightManager::UpdateUBO()
	{
		for (size_t i = 0; i < lights.size(); ++i) {
			glNamedBufferSubData(uboLights, sizeof(GpuLight) * i, sizeof(GpuLight), &lights[i].gpuLight);
		}
		int lightsCount = static_cast<int>(lights.size());
		glNamedBufferSubData(uboLights, sizeof(GpuLight) * MAX_LIGHTS, sizeof(int), &lightsCount);

		shadowManager->UpdateShadows(lights);
	}

	void LightManager::UpdateShadows()
	{
		shadowManager->UpdateShadows(lights);
	}

	void LightManager::BindUBO()
	{
		glBindBufferBase(GL_UNIFORM_BUFFER, 1, uboLights);
	}

	void LightManager::renderUI()
	{
	#if defined(_MYDEBUG)
		bool shouldUpdateBuffer = false;
		// Display each light's properties
		for (int i = 0; i < lights.size(); ++i) {
			AMC::Light& light = lights[i];

			// Collapsible header for each light
			if (ImGui::CollapsingHeader(("Light " + std::to_string(i)).c_str())) {
				// Light type selection
				const char* types[] = { "Directional", "Spot", "Point" };
				int typeIndex = light.gpuLight.type;
				if (ImGui::Combo("Type", &typeIndex, types, IM_ARRAYSIZE(types))) {
					light.gpuLight.type = typeIndex;
					shouldUpdateBuffer = true;
				}

				// Active status
				if (ImGui::Checkbox("Active", reinterpret_cast<bool*>(&light.gpuLight.active))) {
					shouldUpdateBuffer = true;
				}

				// Position controls
				if (ImGui::DragFloat3("Position", &light.gpuLight.position.x, 0.1f)) {
					shouldUpdateBuffer = true;
				}

				// Direction controls (only for Directional and Spot lights)
				if (light.gpuLight.type != LIGHT_TYPE_POINT) {
					if (ImGui::SliderFloat3("Direction", &light.gpuLight.direction.x, -1.0, 1.0f)) {
						shouldUpdateBuffer = true;
					}
				}

				// Color controls
				if (ImGui::DragFloat3("Color", &light.gpuLight.color.x, 0.1f, 0.0f)) {
					shouldUpdateBuffer = true;
				}

				// Intensity control
				if (ImGui::SliderFloat("Intensity", &light.gpuLight.intensity, 0.0f, 10.0f)) {
					shouldUpdateBuffer = true;
				}

				// Range control (for Spot and Point lights)
				if (light.gpuLight.type == LIGHT_TYPE_SPOT || light.gpuLight.type == LIGHT_TYPE_POINT) {
					if (ImGui::SliderFloat("Range", &light.gpuLight.range, 0.0f, 100.0f)) {
						shouldUpdateBuffer = true;
					}
				}

				// Cone angles (only for Spot lights)
				if (light.gpuLight.type == LIGHT_TYPE_SPOT) {

					if (ImGui::SliderFloat("Inner Cone Cosine", &light.gpuLight.spotAngle, 0.0f, 45.0f)) {
						//light.spotAngle = glm::cos(glm::radians(innerConeAngleDegrees));
						shouldUpdateBuffer = true;
					}

					if (ImGui::SliderFloat("Outer Cone Cosine", &light.gpuLight.spotExponent, 0.0f, 45.0f)) {
						//light.spotExponent = glm::cos(glm::radians(outerConeAngleDegrees));
						shouldUpdateBuffer = true;
					}
				}

				// Remove light button
				if (ImGui::Button(("Remove Light " + std::to_string(i)).c_str())) {
					DeleteLight(i);
					--i;
				}
				ImGui::Separator();
			}
		}

		// Button to add a new light
		if (lights.size() < 10) {
			if (ImGui::Button("Add Light")) {
				AMC::Light newLight; // Default new light
				newLight.gpuLight.type = 0;
				newLight.gpuLight.direction = glm::vec3(0.0f, 0.0f, -1.0f);
				newLight.gpuLight.color = glm::vec3(1.0f, 1.0f, 1.0f);
				newLight.gpuLight.intensity = 1.0f;
				newLight.gpuLight.range = 1.0f; // if -1 then range is infinite, used in case of point and spot lights
				newLight.gpuLight.spotAngle = 0.0f; // for spot lights
				newLight.gpuLight.spotExponent = 0.0f; // for spot lights
				newLight.gpuLight.position = glm::vec3(0.0f, 0.0f, 0.0f); // for point and spot lights
				newLight.gpuLight.active = 1; // need to activate light here
				newLight.gpuLight.shadows = true;
				newLight.gpuLight.shadowMapIndex = -1;
				newLight.gpuLight.type = LightType::LIGHT_TYPE_POINT; // need to let shader know what type of light is this
				AddLight(newLight);
				//shouldUpdateBuffer = true;
			}
		}

		ImGui::Separator();

		//ImGui::Text("IBL Settings");
		//if (ImGui::Checkbox("Active", reinterpret_cast<bool*>(&ibl.active))) {
		//	shouldUpdateBuffer = true;
		//}

		//float intensity = ibl.intensity;
		//if (ImGui::SliderFloat("IBL Intensity", &intensity, 0.01f, 100.0f, "%.1f")) {
		//	ibl.intensity = intensity;
		//	shouldUpdateBuffer = true;
		//}

		ImGui::Separator();

		if (shouldUpdateBuffer) {
			UpdateUBO();
		}
	#endif
	}

	void LightManager::drawLights(){

		m_program->use();

		for (const auto& light : lights) {

			if (!light.gpuLight.active) continue;

			glm::mat4 model = glm::mat4(1.0f);

			if (light.gpuLight.type == LIGHT_TYPE_DIRECTIONAL) {
				model = glm::translate(model, light.gpuLight.position);
				glm::vec3 direction = glm::normalize(light.gpuLight.direction);
				glm::quat rotation = glm::rotation(glm::vec3(0.0f, -1.0f, 0.0f), direction);
				glm::mat4 rotMatrix = glm::toMat4(rotation);
				model *= rotMatrix;
			}
			else if (light.gpuLight.type == LIGHT_TYPE_SPOT) {
				model = glm::translate(model, light.gpuLight.position);
				glm::vec3 direction = glm::normalize(light.gpuLight.direction);
				glm::quat rotation = glm::rotation(glm::vec3(0.0f, -1.0f, 0.0f), direction);
				glm::mat4 rotMatrix = glm::toMat4(rotation);
				model *= rotMatrix;
			}
			else if (light.gpuLight.type == LIGHT_TYPE_POINT) {
				model = glm::translate(model, light.gpuLight.position);
			}

			glUniformMatrix4fv(m_program->getUniformLocation("modelMat"), 1, GL_FALSE, glm::value_ptr(model));
			glUniform4fv(m_program->getUniformLocation("color"), 1, glm::value_ptr(light.gpuLight.color));
			if (light.gpuLight.type == LIGHT_TYPE_DIRECTIONAL) {
				directional->draw(m_program, 1, false);
			}
			else if (light.gpuLight.type == LIGHT_TYPE_SPOT) {
				spot->draw(m_program, 1, false);
			}
			else if (light.gpuLight.type == LIGHT_TYPE_POINT) {
				point->draw(m_program, 1, false);
			}
		}
	}
};