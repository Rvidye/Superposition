#include <RenderPass.h>
#include<TextureManager.h>
#include "SuperPosition.h"

void SuperpositionScene::sceneEnd(float t)
{
	if (t > 0.99f)
		completed = true;
}

void SuperpositionScene::init()
{
	// Shader Program Setup

	// ModelPlacer
	mp = new AMC::ModelPlacer(glm::vec3(0.0, 0.0, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), 5.0f);

	// Models Setup
	//AMC::RenderModel cubeobj;
	//cubeobj.model = new AMC::Model(RESOURCE_PATH("models\\BoxAnimated.gltf"), aiProcessPreset_TargetRealtime_Quality | aiProcess_FlipUVs | aiProcess_GenBoundingBoxes);
	//cubeobj.matrix = glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, 0.0f, -10.0f));
	//addModel("cube", cubeobj);

	AMC::RenderModel roomModel;
	roomModel.model = new AMC::Model(RESOURCE_PATH("models\\Sponza1\\Sponza.gltf"), aiProcessPreset_TargetRealtime_Quality | aiProcess_FlipUVs | aiProcess_GenBoundingBoxes);
	roomModel.matrix = mp->getModelMatrix();
	addModel("room", roomModel);

	// Spline Camera Setup
	std::vector<glm::vec3> posVec = {
	{-51.199970f, 4.000000f, 9.000000f},
	{3.200001f, 3.200000f, 15.700002f},
	{39.999981f, 6.100000f, 3.000000f},
	{38.399982f, 5.000000f, -14.200002f},
	{0.800000f, 7.000000f, -15.900002f}
	};
	//std::vector<glm::vec3>  = {
	//	glm::vec3(0.0f, 0.0f, 8.4f),
	//	glm::vec3(0.0f, 0.0f, 2.3f),
	//	glm::vec3(0.0f, -0.6f, 0.5f),
	//	glm::vec3(0.0f, -0.4f, -1.0f),
	//	glm::vec3(0.0f, 0.0f, -3.7f)
	//};

	std::vector<glm::vec3> frontVec = {
	{-53.200027f, 4.500000f, -0.700001f},
	{-39.600002f, 6.300000f, 17.599997f},
	{40.500004f, 7.500000f, 17.599998f},
	{35.999996f, 4.100000f, -20.799997f},
	{0.900000f, 5.400000f, -18.099997f}
	};

	sceneCam = new AMC::SplineCamera(posVec, frontVec);
	camAdjuster = new AMC::SplineCameraAdjuster(sceneCam);

	// event manager setup

	events = new AMC::EventManager();
	//AMC::events_t *endEvent = new AMC::events_t();
	//endEvent->start = 0.0f;
	//endEvent->duration = 10.0f;
	//endEvent->easingFunction = nullptr;
	//endEvent->updateFunction = [this](float t) { this->sceneEnd(t); }; // Bind the member function using a lambda ! did not think this through so here is an ugly hack !!!
	//events->AddEvent("SceneEndEvent", endEvent);

	//AMC::events_t* moveEvent = new AMC::events_t();
	//moveEvent->start = 0.0f;
	//moveEvent->duration = 5.0f;
	//moveEvent->easingFunction = nullptr;
	//moveEvent->updateFunction = [this](float t) { this->moveModel(t); };
	//events->AddEvent("MoveModelEvent", moveEvent);

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
	point.gpuLight.color = glm::vec3(1.0f, 1.0f, 1.0f);
	point.gpuLight.intensity = 0.5f;
	point.gpuLight.range = 25.0f; // range decides the square fall of distance or attenuation of light
	point.gpuLight.spotAngle = 1.0f; // for spot lights
	point.gpuLight.spotExponent = 0.7071f; // for spot lights
	point.gpuLight.position = glm::vec3(0.0f, 0.0f, 0.0f); // for point and spot lights
	point.gpuLight.active = 1; // need to activate light here
	point.gpuLight.shadows = true;
	point.gpuLight.type = AMC::LIGHT_TYPE_POINT; // need to let shader know what type of light is this

	AMC::Light spot;
	spot.gpuLight.direction = glm::vec3(0.0f, 0.0f, 0.0f);
	spot.gpuLight.color = glm::vec3(1.0f, 0.0f, 0.0f);
	spot.gpuLight.intensity = 0.5f;
	spot.gpuLight.range = 25.417f;
	spot.gpuLight.spotAngle = 0.0f; // requrie a cos(radians) doing here just saves computatiaon of GPU
	spot.gpuLight.spotExponent = 45.0f; // requrie a cos(radians) doing here just saves computatiaon of GPU
	spot.gpuLight.position = glm::vec3(9.2f, 10.10f, 27.50f); // for point and spot lights
	spot.gpuLight.active = 1; // need to activate light here
	spot.gpuLight.shadows = false;
	spot.gpuLight.type = AMC::LIGHT_TYPE_POINT; // need to let shader know what type of light is this

	//lightManager->AddLight(spot);
	lightManager->AddLight(point);
	//lightManager->AddLight(directional);
}

//void SuperpositionScene::render()
//{
//	programModel->use();
//	glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(AMC::currentCamera->getProjectionMatrix() * AMC::currentCamera->getViewMatrix() * glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, 0.0f, moveZ))));
//	modelHelmet->draw(programModel);
//
//	programModel->use();
//	glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(AMC::currentCamera->getProjectionMatrix() * AMC::currentCamera->getViewMatrix() * mp->getModelMatrix()));
//	modelAnim->draw(programModel);
//}

void SuperpositionScene::renderDebug()
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

void SuperpositionScene::renderUI()
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
	case AMC::SHADOW:
		lightManager->GetShadowManager()->renderUI();
		break;
	case AMC::SPLINE:
		break;
	case AMC::NONE:
		break;
	}
#endif
}

void SuperpositionScene::update()
{
	events->update();
	//models["cube"].model->update((float)AMC::deltaTime);
	//models["man"].model->update((float)AMC::deltaTime);
	//models["man"].matrix = mp->getModelMatrix();
	reCalculateSceneAABB(); // cannot find better way to do it for now
	//modelAnim->update((float)AMC::deltaTime);
}

void SuperpositionScene::keyboardfunc(char key, UINT keycode)
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

void SuperpositionScene::updateRenderContext(AMC::RenderContext& context)
{
	context.IsVGXI = false;
	context.IsGenerateShadowMaps = true;
	context.IsGbuffer = true;
	context.IsDeferredLighting = true;
	context.IsSSAO = false;
	context.IsSkyBox = true;
	context.IsSSR = false;
	context.IsBloom = false;
	context.IsToneMap = true;
}

AMC::Camera* SuperpositionScene::getCamera()
{
	return sceneCam;
}
