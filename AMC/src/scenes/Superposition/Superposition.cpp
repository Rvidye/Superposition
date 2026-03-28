#include <RenderPass.h>
#include<TextureManager.h>
#include "SuperPosition.h"

void SuperpositionScene::Cam1(float t)
{
	sceneCam->update(t);
	finalCam = sceneCam;
}

void SuperpositionScene::Cam2(float t)
{
	sceneCam1->update(t);
	finalCam = sceneCam1;
}

void SuperpositionScene::Cam3(float t)
{
	sceneCam2->update(t);
	finalCam = sceneCam2;
}

void SuperpositionScene::Cam4(float t)
{
	sceneCam3->update(t);
	finalCam = sceneCam3;
}

void SuperpositionScene::LightRed(float t)
{
	lightManager->GetLight(0)->gpuLight.color = glm::lerp(glm::vec3(500.0f, 500.0f, 500.0f), glm::vec3(500.0f, 0.0f, 0.0f), t);
	lightManager->UpdateUBO();
}

void SuperpositionScene::sceneEnd(float t)
{
	if (t > 0.99f)
		completed = true;
}

void SuperpositionScene::init()
{
	// Shader Program Setup

	// ModelPlacer
	mp = new AMC::ModelPlacer(glm::vec3(0.0, 0.0, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), 1.0f);

	// Models Setup
	//AMC::RenderModel cubeobj;
	//cubeobj.model = new AMC::Model(RESOURCE_PATH("models\\BoxAnimated.gltf"), aiProcessPreset_TargetRealtime_Quality | aiProcess_FlipUVs | aiProcess_GenBoundingBoxes);
	//cubeobj.matrix = glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, 0.0f, -10.0f));
	//addModel("cube", cubeobj);

	AMC::RenderModel roomModel;
	roomModel.model = new AMC::Model(RESOURCE_PATH("models\\Sponza1\\Sponza.gltf"), aiProcessPreset_TargetRealtime_Quality | aiProcess_FlipUVs | aiProcess_GenBoundingBoxes, ctx);
	roomModel.matrix = mp->getModelMatrix();
	addModel("room", roomModel);

	// Spline Camera Setup
	std::vector<glm::vec3> posVec = {
	{3.800030f, 0.100000f, 10.600002f},
	{3.500001f, 0.200001f, 9.199977f},
	{3.500071f, 0.100000f, 7.399996f},
	{3.699978f, -0.000000f, -0.399996f},
	{4.499999f, -0.200000f, -3.600013f}
	};

	std::vector<glm::vec3> frontVec = {
	{15.800056f, -0.800000f, 11.100004f},
	{5.799999f, 0.100001f, 9.499978f},
	{5.700069f, 0.000000f, 7.599996f},
	{3.700071f, 0.000000f, 2.700001f},
	{4.499978f, -0.000000f, -2.699996f},
	{1.399998f, 0.000000f, -3.900013f}
	};

	std::vector<glm::vec3> posVec1 = {
	{-1.899971f, -0.800000f, -4.099995f},
	{-1.699999f, 0.100001f, -4.200027f},
	{-0.899928f, -0.100000f, -3.899999f},
	{-0.100021f, -0.100000f, -3.499995f},
	{0.599979f, -0.100000f, -3.499995f},
	{1.000001f, 0.200000f, -4.100013f}
	};

	std::vector<glm::vec3> frontVec1 = {
	{-4.899937f, -0.400000f, -3.700003f},
	{-10.799995f, -1.500000f, -0.600021f},
	{-7.599927f, 1.300000f, -6.900002f},
	{-0.500022f, 0.700000f, -5.799996f},
	{1.499978f, 0.900000f, -5.799996f},
	{0.699998f, -1.000000f, -3.800014f}
	};

	std::vector<glm::vec3> posVec2 = {
	{-1.799971f, -0.400000f, -2.499997f},
	{-2.099984f, 0.300001f, -4.000025f},
	{-0.899928f, 0.100000f, -4.199998f},
	{1.599979f, 0.000000f, -3.499995f},
	{-0.199999f, -0.300000f, -1.000014f},
	{-0.199999f, -0.300000f, 2.399986f}
	};

	std::vector<glm::vec3> frontVec2 = {
	{-0.899939f, -0.400000f, -3.400003f},
	{1.700009f, -2.000000f, -3.900019f},
	{-0.799933f, -0.600000f, -3.800004f},
	{-1.900022f, -0.700000f, -3.299998f},
	{-0.600002f, -0.200000f, -5.600009f},
	{-0.600002f, -0.200000f, -14.000030f}
	};

	std::vector<glm::vec3> posvec3 = {
	{1.600029f, -1.000000f, 4.700001f},
	{0.000072f, 0.000000f, 5.799997f},
	{-2.300021f, 0.000000f, 3.800003f},
	{-0.299999f, -0.100000f, 0.799986f},
	{-0.599999f, -1.200000f, -0.500014f}
	};

	std::vector<glm::vec3> frontvec3 = {
	{-0.399939f, 0.400000f, 4.599994f},
	{0.900067f, -0.300000f, 5.499992f},
	{-1.000022f, -0.700000f, 6.099998f},
	{-3.000001f, 0.100000f, 3.599987f},
	{-0.600002f, -0.100000f, 4.199986f}
	};


	sceneCam = new AMC::SplineCamera(posVec, frontVec);
	sceneCam1 = new AMC::SplineCamera(posVec1, frontVec1);
	sceneCam2 = new AMC::SplineCamera(posVec2, frontVec2);
	sceneCam3 = new AMC::SplineCamera(posvec3, frontvec3);
	//sceneCam4 = new AMC::SplineCamera(posVec2, frontVec2);
	//sceneCam5 = new AMC::SplineCamera(posVec2, frontVec2);
	camAdjuster = new AMC::SplineCameraAdjuster(sceneCam3);
	//finalCam = sceneCam3;

	// event manager setup

	events = new AMC::EventManager();
	AMC::events_t *endEvent = new AMC::events_t();
	endEvent->start = 0.0f;
	endEvent->duration = 325.0f;
	endEvent->easingFunction = nullptr;
	endEvent->updateFunction = [this](float t) { this->sceneEnd(t); }; // Bind the member function using a lambda ! did not think this through so here is an ugly hack !!!
	events->AddEvent("SceneEndEvent", endEvent);

	AMC::events_t* camevent1 = new AMC::events_t();
	camevent1->start = 0.0f;
	camevent1->duration = 10.0f;
	camevent1->easingFunction = nullptr;
	camevent1->updateFunction = [this](float t) { this->Cam1(t); };
	events->AddEvent("Camera1", camevent1);

	AMC::events_t* camevent2 = new AMC::events_t();
	camevent2->start = 11.0f;
	camevent2->duration = 30.0f;
	camevent2->easingFunction = nullptr;
	camevent2->updateFunction = [this](float t) { this->Cam2(t); };
	events->AddEvent("Camera2", camevent2);

	AMC::events_t* camevent3 = new AMC::events_t();
	camevent3->start = 41.0f;
	camevent3->duration = 30.0f;
	camevent3->easingFunction = nullptr;
	camevent3->updateFunction = [this](float t) { this->Cam3(t); };
	events->AddEvent("Camera3", camevent3);

	AMC::events_t* camevent4 = new AMC::events_t();
	camevent4->start = 72.0f;
	camevent4->duration = 25.0f;
	camevent4->easingFunction = nullptr;
	camevent4->updateFunction = [this](float t) { this->Cam4(t); };
	events->AddEvent("Camera4", camevent4);

	AMC::events_t* LightEvent = new AMC::events_t();
	LightEvent->start = 97.0f;
	LightEvent->duration = 10.0f;
	LightEvent->easingFunction = nullptr;
	LightEvent->updateFunction = [this](float t) { this->LightRed(t); };
	events->AddEvent("Light1", LightEvent);

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
	point.gpuLight.color = glm::vec3(500.0f, 500.0f, 500.0f);
	point.gpuLight.intensity = 0.5f;
	point.gpuLight.range = 0.15; // range decides the square fall of distance or attenuation of light
	point.gpuLight.spotAngle = 1.0f; // for spot lights
	point.gpuLight.spotExponent = 0.7071f; // for spot lights
	point.gpuLight.position = glm::vec3(-4.0f, 3.90f, 0.0f); // for point and spot lights
	point.gpuLight.active = 1; // need to activate light here
	point.gpuLight.shadows = true;
	point.gpuLight.type = AMC::LIGHT_TYPE_POINT; // need to let shader know what type of light is this

	AMC::Light spot;
	spot.gpuLight.direction = glm::vec3(0.0f, 0.0f, 0.0f);
	spot.gpuLight.color = glm::vec3(100.0f, 0.0f, 0.0f);
	spot.gpuLight.intensity = 0.5f;
	spot.gpuLight.range = 0.15f;
	spot.gpuLight.spotAngle = 0.0f; // requrie a cos(radians) doing here just saves computatiaon of GPU
	spot.gpuLight.spotExponent = 45.0f; // requrie a cos(radians) doing here just saves computatiaon of GPU
	spot.gpuLight.position = glm::vec3(0.0f, 0.0f, 0.0f); // for point and spot lights
	spot.gpuLight.active = 1; // need to activate light here
	spot.gpuLight.shadows = true;
	spot.gpuLight.type = AMC::LIGHT_TYPE_POINT; // need to let shader know what type of light is this

	lightManager->AddLight(point);
	//lightManager->AddLight(spot);
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
	ImGui::Text("Superposition Scene ");
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

void SuperpositionScene::update()
{
	events->update();
	//models["cube"].model->update((float)AMC::deltaTime);
	//models["man"].model->update((float)AMC::deltaTime);
	models["room"].matrix = mp->getModelMatrix();
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
	if (!OverrideRenderer)
		return;
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
	return finalCam;
}
