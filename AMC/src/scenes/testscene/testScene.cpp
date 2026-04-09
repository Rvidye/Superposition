#include "testScene.h"

#include<RenderPass.h>
#include<HairMeshAsset.h>

void testScene::sceneEnd(float t)
{
	if (t > 0.99f)
		completed = true;
}

void testScene::moveModel(float t)
{
	//moveZ = std::lerp(-15.0f, -5.0f, t);
	//models["cube"].matrix = glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, 0.0f, moveZ));
}

void testScene::fadeOut(float t)
{
	AMC::fade = std::lerp(AMC::fade, 1.0f, t);
}

void testScene::fadeIn(float t)
{
	AMC::fade = std::lerp(AMC::fade, 0.0f, t);
}

void testScene::init()
{
	// Shader Program Setup
	programModel = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\model\\model.vert"),RESOURCE_PATH("shaders\\model\\model.frag") });
	programModelAnim = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\model\\modelAnim.vert"),RESOURCE_PATH("shaders\\model\\model.frag") });
	hairDebugProgram = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\color\\color.vert"), RESOURCE_PATH("shaders\\color\\color.frag") });

	glCreateVertexArrays(1, &hairDebugVAO);
	glCreateBuffers(1, &hairDebugVBO);
	glNamedBufferData(hairDebugVBO, sizeof(glm::vec4), nullptr, GL_DYNAMIC_DRAW);
	glVertexArrayVertexBuffer(hairDebugVAO, 0, hairDebugVBO, 0, sizeof(glm::vec4));
	glEnableVertexArrayAttrib(hairDebugVAO, 0);
	glVertexArrayAttribFormat(hairDebugVAO, 0, 4, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(hairDebugVAO, 0, 0);

	// ModelPlacer
	mp = new AMC::ModelPlacer(glm::vec3(0.0, 0.0, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), 1.0f);

	// Models Setup
	//AMC::RenderModel cubeobj;
	//cubeobj.model = new AMC::Model(RESOURCE_PATH("models\\BoxAnimated.gltf"), aiProcessPreset_TargetRealtime_Quality | aiProcess_FlipUVs | aiProcess_GenBoundingBoxes);
	//cubeobj.matrix = glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, 0.0f, -10.0f));
	//addModel("cube", cubeobj);

	AMC::RenderModel animman;
	animman.model = new AMC::Model(RESOURCE_PATH("models\\DamagedHelmet\\DamagedHelmet.gltf"), aiProcessPreset_TargetRealtime_Quality | aiProcess_FlipUVs | aiProcess_GenBoundingBoxes, ctx);
	animman.matrix = mp->getModelMatrix();
	addModel("man", animman);

	const glm::mat4 sweptLeftMatrix =
		glm::translate(glm::mat4(1.0f), glm::vec3(-0.32f, 1.48f, 0.10f)) *
		glm::rotate(glm::mat4(1.0f), glm::radians(-16.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	const glm::mat4 crownBobMatrix =
		glm::translate(glm::mat4(1.0f), glm::vec3(0.00f, 1.52f, 0.00f));
	const glm::mat4 ponytailMatrix =
		glm::translate(glm::mat4(1.0f), glm::vec3(0.34f, 1.54f, -0.08f)) *
		glm::rotate(glm::mat4(1.0f), glm::radians(12.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	hairs["hair_swept_left"] = {
		new AMC::HairMeshAsset(RESOURCE_PATH("hair\\authoring\\swept_left.json")),
		sweptLeftMatrix,
		sweptLeftMatrix,
		true
	};
	hairs["hair_crown_bob"] = {
		new AMC::HairMeshAsset(RESOURCE_PATH("hair\\authoring\\crown_bob.json")),
		crownBobMatrix,
		crownBobMatrix,
		true
	};
	hairs["hair_ponytail"] = {
		new AMC::HairMeshAsset(RESOURCE_PATH("hair\\authoring\\ponytail_fan.json")),
		ponytailMatrix,
		ponytailMatrix,
		true
	};

	// Spline Camera Setup
	std::vector<glm::vec3> posVec = {
		glm::vec3(0.0f, 0.0f, 8.4f),
		glm::vec3(0.0f, 0.0f, 2.3f),
		glm::vec3(0.0f, -0.6f, 0.5f),
		glm::vec3(0.0f, -0.4f, -1.0f),
		glm::vec3(0.0f, 0.0f, -3.7f)
	};

	std::vector<glm::vec3> frontVec = {
		glm::vec3(0.0f, 0.0f, 9.8f),
		glm::vec3(0.0f, 0.0f, 3.7f),
		glm::vec3(0.0f, 0.5f, 2.2f),
		glm::vec3(0.0f, -0.6f, 1.0f),
		glm::vec3(0.0f, -0.2f, -0.7f)
	};
	
	sceneCam = new AMC::SplineCamera(posVec,frontVec);
	camAdjuster = new AMC::SplineCameraAdjuster(sceneCam);

	// event manager setup

	events = new AMC::EventManager();
	//AMC::events_t *endEvent = new AMC::events_t();
	//endEvent->start = 0.0f;
	//endEvent->duration = 10.0f;
	//endEvent->easingFunction = nullptr;
	//endEvent->updateFunction = [this](float t) { this->sceneEnd(t); }; // Bind the member function using a lambda ! did not think this through so here is an ugly hack !!!
	//events->AddEvent("SceneEndEvent", endEvent);

	AMC::events_t* moveEvent = new AMC::events_t();
	moveEvent->start = 0.0f;
	moveEvent->duration = 5.0f;
	moveEvent->easingFunction = nullptr;
	moveEvent->updateFunction = [this](float t) { this->moveModel(t); };
	events->AddEvent("MoveModelEvent", moveEvent);

	AMC::events_t* fade1 = new AMC::events_t();
	fade1->start = 5.0f;
	fade1->duration = 1.0f;
	fade1->easingFunction = nullptr;
	fade1->updateFunction = [this](float t) { this->fadeOut(t); };
	events->AddEvent("fade1", fade1);

	AMC::events_t* fade2 = new AMC::events_t();
	fade2->start = 7.0f;
	fade2->duration = 1.0f;
	fade2->easingFunction = nullptr;
	fade2->updateFunction = [this](float t) { this->fadeIn(t); };
	events->AddEvent("fade2", fade2);

	AMC::events_t* fade3 = new AMC::events_t();
	fade3->start = 10.0f;
	fade3->duration = 1.0f;
	fade3->easingFunction = nullptr;
	fade3->updateFunction = [this](float t) { this->fadeOut(t); };
	events->AddEvent("fade3", fade3);

	AMC::events_t* fade4 = new AMC::events_t();
	fade4->start = 12.0f;
	fade4->duration = 1.0f;
	fade4->easingFunction = nullptr;
	fade4->updateFunction = [this](float t) { this->fadeIn(t); };
	events->AddEvent("fade4", fade4);

	lightManager = new AMC::LightManager();

	AMC::Light directional;
	directional.gpuLight.direction = glm::vec3(0.50f, -0.7071f, -0.50f);
	directional.gpuLight.color = glm::vec3(1.0f, 1.0f, 1.0f);
	directional.gpuLight.intensity = 1.0f;
	directional.gpuLight.range = 7.5f; // if 0.0 then range is infinite, used in case of point and spot lights
	directional.gpuLight.spotAngle = 1.0f; // for spot lights
	directional.gpuLight.spotExponent = 0.7071f; // for spot lights
	directional.gpuLight.position = glm::vec3(-19.40f, 6.2f, -21.10f); // for point and spot lights
	directional.gpuLight.active = 1; // need to activate light here
	directional.gpuLight.shadows = false;
	directional.gpuLight.type = AMC::LIGHT_TYPE_POINT; // need to let shader know what type of light is this

	AMC::Light point;
	point.gpuLight.direction = glm::vec3(-0.50f, 0.7071f, 0.50f); // doesn't matter in case of point lights
	point.gpuLight.color = glm::vec3(500.0f, 0.0f, 0.0f);
	point.gpuLight.intensity = 0.5f;
	point.gpuLight.range = 0.15f; // range decides the square fall of distance or attenuation of light
	point.gpuLight.spotAngle = 1.0f; // for spot lights
	point.gpuLight.spotExponent = 0.7071f; // for spot lights
	point.gpuLight.position = glm::vec3(-4.0f, 3.90f, 0.0f); // for point and spot lights
	point.gpuLight.active = 1; // need to activate light here
	point.gpuLight.shadows = true;
	point.gpuLight.type = AMC::LIGHT_TYPE_POINT; // need to let shader know what type of light is this

	AMC::Light spot;
	spot.gpuLight.direction = glm::vec3(0.0f, 0.0f, 0.0f);
	spot.gpuLight.color = glm::vec3(500.0f, 500.0f, 500.0f);
	spot.gpuLight.intensity = 0.5f;
	spot.gpuLight.range = 0.150f;
	spot.gpuLight.spotAngle = 0.0f; // requrie a cos(radians) doing here just saves computatiaon of GPU
	spot.gpuLight.spotExponent = 45.0f; // requrie a cos(radians) doing here just saves computatiaon of GPU
	spot.gpuLight.position = glm::vec3(4.0f, 3.90f, 0.0f); // for point and spot lights
	spot.gpuLight.active = 1; // need to activate light here
	spot.gpuLight.shadows = true;
	spot.gpuLight.type = AMC::LIGHT_TYPE_POINT; // need to let shader know what type of light is this

	lightManager->AddLight(point);
	lightManager->AddLight(spot);
	//lightManager->AddLight(directional);
	reCalculateSceneAABB();
}

//void testScene::render()
//{
//	programModel->use();
//	glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(AMC::currentCamera->getProjectionMatrix() * AMC::currentCamera->getViewMatrix() * glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, 0.0f, moveZ))));
//	modelHelmet->draw(programModel);
//
//	programModel->use();
//	glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(AMC::currentCamera->getProjectionMatrix() * AMC::currentCamera->getViewMatrix() * mp->getModelMatrix()));
//	modelAnim->draw(programModel);
//}

void testScene::renderDebug()
{
	switch (AMC::DEBUGMODE) {
		case AMC::MODEL:
			break;
		case AMC::CAMERA:
			camAdjuster->render();
			break;
		case AMC::LIGHT:
			lightManager->drawLights();
			break;
		case AMC::SPLINE:
			break;
		case AMC::NONE:
			break;
	}
}

void testScene::renderHairDebug(AMC::RenderContext& context)
{
	if (!context.IsHairDebugWireframe || hairDebugProgram == nullptr || hairDebugVAO == 0) {
		return;
	}

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glLineWidth(1.5f);

	hairDebugProgram->use();
	glUniform4f(2, 0.1f, 0.9f, 1.0f, 1.0f);
	glBindVertexArray(hairDebugVAO);

	for (const auto& [name, hair] : hairs) {
		if (!hair.visible || hair.asset == nullptr) {
			continue;
		}

		const auto& lines = hair.asset->GetDebugLineVertices();
		if (lines.empty()) {
			continue;
		}

		const glm::mat4 mvp = AMC::currentCamera->getProjectionMatrix() * AMC::currentCamera->getViewMatrix() * hair.matrix;
		glNamedBufferData(hairDebugVBO, static_cast<GLsizeiptr>(lines.size() * sizeof(glm::vec4)), lines.data(), GL_DYNAMIC_DRAW);
		glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(mvp));
		glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(lines.size()));
	}
}

void testScene::renderUI()
{
	#if defined(_MYDEBUG)
	ImGui::Text("Tutorial Scene ");
	ImGui::Text("Scene Time : %0.1f", events->getCurrentTime());
	switch (AMC::DEBUGMODE) {
		case AMC::MODEL:
			mp->renderUI();
		break;
		case AMC::CAMERA:
			camAdjuster->renderUI();
		break;
		case AMC::LIGHT:
			lightManager->renderUI();
		break;
		case AMC::SPLINE:
		break;
		case AMC::NONE:
		break;
	}
	#endif
}

void testScene::update()
{
	events->update();
	//models["cube"].model->update((float)AMC::deltaTime);
	models["man"].model->update((float)AMC::deltaTime);
	models["man"].matrix = mp->getModelMatrix();
	reCalculateSceneAABB(); // cannot find better way to do it for now
	//modelAnim->update((float)AMC::deltaTime);
}

void testScene::keyboardfunc(char key, UINT keycode)
{
	switch (AMC::DEBUGMODE) {
		case AMC::MODEL:
			mp->keyboardfunc(key);
		break;
		case AMC::CAMERA:
			camAdjuster->keyboardfunc(key, keycode);
		break;
		case AMC::LIGHT:
		break;
		case AMC::SPLINE:
		break;
		case AMC::NONE:
		break;
	}
}

void testScene::updateRenderContext(AMC::RenderContext& context)
{
}

AMC::Camera* testScene::getCamera()
{
	return sceneCam;
}
