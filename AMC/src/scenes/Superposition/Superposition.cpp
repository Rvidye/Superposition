#include <RenderPass.h>
#include<TextureManager.h>
#include "SuperPosition.h"

void SuperpositionScene::Cam1(float t)
{
	sceneCam->update(t);
	AMC::GlobalGIBoost = std::lerp(AMC::GlobalGIBoost, 1.3f, t);
	AMC::AtmosphericElevation = std::lerp(AMC::AtmosphericElevation, 1.0f, t);
	finalCam = sceneCam;
}

void SuperpositionScene::Cam2(float t)
{
	sceneCam1_a->update(t);
	finalCam = sceneCam1_a;
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

void SuperpositionScene::Cam5(float t)
{
	sceneCam4->update(t);
	finalCam = sceneCam4;
}

void SuperpositionScene::Cam6(float t)
{
	sceneCam5->update(t);
	finalCam = sceneCam5;
}

void SuperpositionScene::Cam7(float t)
{
	sceneCam6->update(t);
	finalCam = sceneCam6;
}

void SuperpositionScene::Apple(float t)
{
	models["apple"].model->lerpAnimation(std::lerp(1.0f, 0.0f, t));
	finalCam = sceneCam4;
}

void SuperpositionScene::LightRed(float t)
{
	lightManager->GetLight(1)->gpuLight.color = glm::lerp(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(300.0f, 0.0f, 0.0f), t);
	lightManager->GetLight(1)->gpuLight.position = glm::lerp(glm::vec3(-0.200f, -2.0f, 4.60f), glm::vec3(-0.200f, 1.3f, 4.60f), t);
	lightManager->UpdateUBO();
	AMC::VolumeStength = 0.792f;
	AMC::VolumeScattering = 0.826;
}

void SuperpositionScene::MachineStart(float t)
{
	lightManager->GetLight(1)->gpuLight.range = std::lerp(0.650f, 10.0f, t);
	lightManager->UpdateUBO();
}

void SuperpositionScene::MachineStop(float t)
{
	lightManager->GetLight(1)->gpuLight.range = std::lerp(10.f, 0.200f, t);
	lightManager->UpdateUBO();
}

void SuperpositionScene::ObjectFloat(float t)
{
	models["roomAnimated"].model->lerpAnimation(std::lerp(0.0f, 0.65f, t));
}

void SuperpositionScene::ObjectFall(float t)
{
	models["roomAnimated"].model->lerpAnimation(std::lerp(0.65f, 0.99f, t));
}

void SuperpositionScene::ObjectReverse(float t)
{
	models["roomAnimated"].model->lerpAnimation(std::lerp(0.99f, 0.00f, t));
}

void SuperpositionScene::sceneEnd(float t)
{
	if (t > 0.99f)
		completed = true;
}

void SuperpositionScene::init()
{
	// Shader Program Setup
	OverrideRenderer = true;
	// ModelPlacer
	mp = new AMC::ModelPlacer(glm::vec3(0.9f, -0.4f, -3.6f), glm::vec3(0.0f, 0.0f, 0.0f), 0.20f);

	// Models Setup
	AMC::RenderModel apple;
	apple.model = new AMC::Model(RESOURCE_PATH("models\\Apple\\Apple.gltf"), aiProcessPreset_TargetRealtime_Quality | aiProcess_FlipUVs | aiProcess_GenBoundingBoxes);
	apple.matrix = glm::translate(glm::mat4(1.0f), glm::vec3(-2.400000f, -0.517000f, 1.400000f)) * glm::yawPitchRoll(0.000000f, 0.000000f, -0.890118f) * glm::scale(glm::mat4(1.0f), glm::vec3(0.200000f));//mp->getModelMatrix();//glm::translate(glm::mat4(1.0f), glm::vec3(0.9f, -0.4f, -3.6f)) * glm::yawPitchRoll(0.0f, 0.0f, 0.0f) * glm::scale(glm::mat4(1.0f), glm::vec3(0.20f));//mp->getModelMatrix();
	addModel("apple", apple);

	AMC::RenderModel roomModel;
	roomModel.model = new AMC::Model(RESOURCE_PATH("models\\\SP1\\SuperPositionStatic.gltf"), aiProcessPreset_TargetRealtime_Quality | aiProcess_FlipUVs | aiProcess_GenBoundingBoxes);
	roomModel.matrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.5f, -1.7f, 1.2f));
	addModel("roomStatic", roomModel);

	AMC::RenderModel roomAnimated;
	roomAnimated.model = new AMC::Model(RESOURCE_PATH("models\\SP1\\SuperPositionAnimation.gltf"), aiProcessPreset_TargetRealtime_Quality | aiProcess_FlipUVs | aiProcess_GenBoundingBoxes);
	roomAnimated.model->lerpAnimation(0.01f);
	roomAnimated.matrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.5f, -1.7f, 1.2f)) * glm::yawPitchRoll(0.0f, 0.0f, 0.0f) * glm::scale(glm::mat4(1.0f), glm::vec3(1.0f));
	addModel("roomAnimated", roomAnimated);

	//AMC::RenderModel model2;
	//model2.model = new AMC::Model(RESOURCE_PATH("models\\RasterLogo\\RasterLogo.gltf"), aiProcessPreset_TargetRealtime_Quality | aiProcess_FlipUVs | aiProcess_GenBoundingBoxes);
	//model2.matrix = mp->getModelMatrix();//glm::translate(glm::mat4(1.0f), glm::vec3(0.5f, -1.7f, 1.2f));
	//addModel("logo", model2);

	// Spline Camera Setup
	std::vector<glm::vec3> posVec = {
	{4.100000f, 0.000001f, 3.599977f},
	{4.200070f, 0.000000f, 2.200001f},
	{4.399978f, -0.000000f, -1.799996f},
	{4.499999f, 0.000000f, -3.600013f}
	};
	/*{
	{3.800030f, 0.100000f, 10.600002f},
	{3.500001f, 0.200001f, 9.199977f},
	{3.500071f, 0.100000f, 7.399996f},
	{3.699978f, -0.000000f, -0.399996f},
	{4.499999f, -0.200000f, -3.600013f}
	};*/

	std::vector<glm::vec3> frontVec = {
	{4.100070f, -0.100000f, 2.200001f},
	{4.100071f, -0.100000f, -0.799999f},
	{3.999979f, -0.100000f, -2.699996f},
	{1.299998f, -0.000000f, -3.500013f}
	};
	/*{
	{15.800056f, -0.800000f, 11.100004f},
	{5.799999f, 0.100001f, 9.499978f},
	{5.700069f, 0.000000f, 7.599996f},
	{3.700071f, 0.000000f, 2.700001f},
	{4.499978f, -0.000000f, -2.699996f},
	{1.399998f, 0.000000f, -3.900013f}
	};*/

	std::vector<glm::vec3> posVec1 = {
	{-1.099971f, -0.400000f, -4.699995f},
	{-1.399999f, -0.199999f, -3.900027f},
	{-0.799928f, 0.200000f, -2.200001f},
	{1.299979f, -0.100000f, -1.099996f}
	};
	/*{
	{-1.099971f, -0.400000f, -4.699995f},
	{-1.699999f, 0.100001f, -4.200027f},
	{-0.899928f, -0.100000f, -3.899999f},
	{-0.100021f, -0.100000f, -3.499995f},
	{0.599979f, -0.100000f, -3.499995f},
	{1.300001f, 0.200000f, -3.900013f}
	};*/

	std::vector<glm::vec3> frontVec1 = {
	{-1.899940f, -0.400000f, -4.600002f},
	{-2.999990f, -0.100000f, -4.100019f},
	{-2.599931f, 0.800000f, -5.100004f},
	{-1.100022f, 0.600000f, -5.499996f}
	};
	/*{
	{-4.399938f, -0.700000f, -4.500002f},
	{-10.799995f, -1.500000f, -0.600021f},
	{-7.599927f, 1.300000f, -6.900002f},
	{-0.500022f, 0.700000f, -5.799996f},
	{1.499978f, 0.900000f, -5.799996f},
	{1.099998f, -1.100000f, -3.600014f}
	};*/

	std::vector<glm::vec3> posVec2 = {
	{-0.999971f, -0.200000f, -3.899996f},
	{-0.399984f, -0.099999f, -4.200025f},
	{0.600072f, 0.100000f, -4.199998f},
	{1.999979f, 0.100000f, -3.499995f},
	{1.399979f, 0.100000f, -2.499996f},
	{-0.199999f, -0.300000f, -1.000014f},
	{-0.199999f, -0.600000f, 2.299986f}
	};
	/*{
	{-1.799971f, -0.700000f, -2.499997f},
	{-2.099984f, 0.300001f, -4.000025f},
	{-0.899928f, 0.100000f, -4.199998f},
	{2.399979f, 0.000000f, -3.499995f},
	{-0.199999f, -0.300000f, -1.000014f},
	{-0.199999f, -0.600000f, 2.299986f}
	};*/

	std::vector<glm::vec3> frontVec2 = {
	{-0.699939f, -0.300000f, -3.600003f},
	{-0.499991f, -0.300000f, -3.400020f},
	{-0.499991f, -0.300000f, -3.600020f},
	{0.100067f, -0.200000f, -3.500004f},
	{0.299978f, 0.100000f, -3.699998f},
	{0.199998f, -0.100000f, -4.800010f},
	{-0.600002f, -1.500000f, -14.000030f}
	};
	/*{
	{-0.899939f, -0.700000f, -3.400003f},
	{-1.299991f, -0.300000f, -5.400018f},
	{-0.099933f, -0.900000f, -2.600005f},
	{-1.900022f, -0.700000f, -3.299998f},
	{-0.600002f, -0.200000f, -5.600009f},
	{-0.600002f, -1.500000f, -14.000030f}
	};*/

	std::vector<glm::vec3> posvec3 = {
	{1.900029f, -1.000000f, 4.900001f},
	{1.000072f, 0.300000f, 6.399996f},
	{-1.199928f, -0.000000f, 5.799997f},
	{-2.300021f, 0.000000f, 3.800003f},
	{-0.299999f, -0.100000f, 0.799986f},
	{-0.199999f, -1.200000f, -0.600014f}
	};
	/*{
	{1.900029f, -1.000000f, 4.900001f},
	{0.000072f, 0.000000f, 5.799997f},
	{-2.300021f, 0.000000f, 3.800003f},
	{-0.299999f, -0.100000f, 0.799986f},
	{-0.199999f, -1.200000f, -0.600014f}
	};*/

	std::vector<glm::vec3> frontvec3 = {
	{1.000061f, -0.400000f, 4.899994f},
	{0.500067f, -0.000000f, 4.899992f},
	{-0.300022f, 0.600000f, 4.300000f},
	{-1.400022f, -0.100000f, 4.300000f},
	{-1.400002f, -0.300000f, 3.899987f},
	{-0.300002f, 0.200000f, 4.199986f}
	};
	/*{
	{-0.299939f, 0.400000f, 4.699994f},
	{0.900067f, -0.300000f, 5.499992f},
	{-1.000022f, -0.700000f, 6.099998f},
	{-3.000001f, 0.100000f, 3.599987f},
	{-0.600002f, -0.200000f, 4.199986f}
	};*/

	std::vector<glm::vec3> posvec4 = {
	{-1.599971f, -0.200000f, 1.200003f},
	{-1.499971f, -0.300000f, 1.300003f},
	{-1.099971f, -0.300000f, 1.200003f},
	{-0.699999f, -0.499999f, 0.999971f},
	{-0.299928f, -0.600000f, 0.900000f},
	{0.299979f, -0.700000f, 0.700004f},
	{0.199979f, -0.700000f, 1.500004f}
	};
	/*{
	{0.900029f, -0.200000f, -4.399995f},
	{1.400001f, -0.299999f, -2.400029f},
	{0.300072f, -0.400000f, -0.900000f},
	{0.099979f, 0.500000f, 1.500004f}
	};*/

	std::vector<glm::vec3> frontvec4 = {
	{-3.199939f, -0.300000f, 2.499996f},
	{-2.499940f, -0.400000f, 1.699996f},
	{-2.299940f, -0.400000f, 1.399996f},
	{-2.299940f, -0.400000f, 0.999996f},
	{-2.199940f, -0.300000f, -0.100004f},
	{-1.799991f, -0.200000f, -2.100022f},
	{-0.999932f, -0.100000f, -3.500006f},
	{0.099978f, -0.000000f, -4.399997f}
	};
	/*{
	{0.900060f, -0.300000f, 2.599996f},
	{-2.099991f, 0.000000f, -5.400019f},
	{-0.799932f, -0.300000f, -7.200002f},
	{-0.100022f, -0.500000f, -4.399997f}
	};*/

	std::vector<glm::vec3> posvec5 = {
	{0.199979f, -0.700000f, 1.500004f},
	{1.400072f, 0.200000f, -2.000000f},
	{0.900001f, 0.900001f, -3.800028f},
	{-1.099971f, 0.800000f, -4.499995f},
	{-2.299971f, 0.200000f, -2.399997f},
	{-1.599971f, -0.000000f, -0.499997f}
	};
	//{
	//// {-0.500021f, 1.200000f, -0.999996f},
	//{0.199979f, -0.700000f, 1.500004f},
	//{2.000072f, 0.900000f, -2.100000f},
	//{1.900001f, 0.900001f, -3.800028f},
	//{-1.699971f, 0.600000f, -4.499995f},
	//{-1.599971f, -0.500000f, -0.499997f}
	//};

	std::vector<glm::vec3> frontvec5 = {
	{0.099978f, -0.000000f, -4.399997f},
	{-0.399932f, 0.400000f, -3.500006f},
	{-0.099991f, 0.500000f, -3.000021f},
	{0.700060f, 0.500000f, -4.700002f}
	};
	//{
	//// {0.499978f, 0.300000f, -4.699997f},
	//{0.099978f, -0.000000f, -4.399997f},
	//{-2.899932f, -0.100000f, -4.500005f},
	//{-2.599991f, 0.000000f, -2.300022f},
	//{0.700060f, -0.100000f, -5.800001f}
	//};

	std::vector<glm::vec3> posvec6 = {
	{-2.69f, -0.24f, -2.2f},
	{-2.26f, -0.13f, -1.3f},
	{-1.82f, -0.03f, -0.41f},
	{-1.6f, 0.02f, 0.04f}
	};

	std::vector<glm::vec3> frontvec6 = {
	{-2.26f, -0.13f, -1.3f},
	{-1.82f, -0.03f, -0.41f},
	{-1.6f, 0.02f, 0.04f},
	{-1.39f, 0.08f, 0.48f}
	};


	sceneCam = new AMC::SplineCamera(posVec, frontVec);
	sceneCam1_a = new AMC::SplineCamera(posVec1, frontVec1);
	sceneCam2 = new AMC::SplineCamera(posVec2, frontVec2);
	sceneCam3 = new AMC::SplineCamera(posvec3, frontvec3);
	sceneCam4 = new AMC::SplineCamera(posvec4, frontvec4);
	sceneCam5 = new AMC::SplineCamera(posvec5, frontvec5);
	sceneCam6 = new AMC::SplineCamera(posvec6, frontvec6);
	camAdjuster = new AMC::SplineCameraAdjuster(sceneCam2);
	finalCam = sceneCam2;

	// event manager setup
	float sequence = 0.0f;

	events = new AMC::EventManager();
	AMC::events_t *endEvent = new AMC::events_t();
	endEvent->start = 0.0f;
	endEvent->duration = 325.0f;
	endEvent->easingFunction = nullptr;
	endEvent->updateFunction = [this](float t) { this->sceneEnd(t); }; // Bind the member function using a lambda ! did not think this through so here is an ugly hack !!!
	events->AddEvent("SceneEndEvent", endEvent);

	AMC::events_t* camevent1 = new AMC::events_t();
	camevent1->start = sequence;
	camevent1->duration = 15.0f;
	camevent1->easingFunction = nullptr;
	camevent1->updateFunction = [this](float t) { this->Cam1(t); };
	events->AddEvent("Camera1", camevent1);

	sequence = camevent1->start + camevent1->duration;

	AMC::events_t* camevent2 = new AMC::events_t();
	camevent2->start = sequence;
	camevent2->duration = 18.0f;
	camevent2->easingFunction = nullptr;
	camevent2->updateFunction = [this](float t) { this->Cam2(t); };
	events->AddEvent("Camera2", camevent2);

	sequence = camevent2->start + camevent2->duration;

	AMC::events_t* camevent3 = new AMC::events_t();
	camevent3->start = sequence + 1.0f;
	camevent3->duration = 30.0f;
	camevent3->easingFunction = nullptr;
	camevent3->updateFunction = [this](float t) { this->Cam3(t); };
	events->AddEvent("Camera3", camevent3);

	sequence = camevent3->start + camevent3->duration;

	AMC::events_t* camevent4 = new AMC::events_t();
	camevent4->start = sequence + 1.0f;
	camevent4->duration = 35.0f;
	camevent4->easingFunction = nullptr;
	camevent4->updateFunction = [this](float t) { this->Cam4(t); };
	events->AddEvent("Camera4", camevent4);

	sequence = camevent4->start + camevent4->duration;

	AMC::events_t* LightEvent = new AMC::events_t();
	LightEvent->start = sequence + 1.0f;
	LightEvent->duration = 19.0f;
	LightEvent->easingFunction = nullptr;
	LightEvent->updateFunction = [this](float t) { this->LightRed(t); };
	events->AddEvent("Light1", LightEvent);

	sequence = LightEvent->start + LightEvent->duration;

	AMC::events_t* MachineEvent1 = new AMC::events_t();
	MachineEvent1->start = sequence;
	MachineEvent1->duration = 2.0f;
	MachineEvent1->easingFunction = nullptr;
	MachineEvent1->updateFunction = [this](float t) { this->MachineStart(t); };
	events->AddEvent("Light2", MachineEvent1);

	sequence = MachineEvent1->start + MachineEvent1->duration;

	AMC::events_t* MachineEvent2 = new AMC::events_t();
	MachineEvent2->start = sequence;
	MachineEvent2->duration = 2.0f;
	MachineEvent2->easingFunction = nullptr;
	MachineEvent2->updateFunction = [this](float t) { this->MachineStop(t); };
	events->AddEvent("Light3", MachineEvent2);

	sequence = MachineEvent2->start + MachineEvent2->duration;

	AMC::events_t* appleevent = new AMC::events_t();
	appleevent->start = sequence + 1.0f;
	appleevent->duration = 10.0f;
	appleevent->easingFunction = nullptr;
	appleevent->updateFunction = [this](float t) { this->Apple(t); };
	events->AddEvent("appleevent", appleevent); 

	sequence = appleevent->start + appleevent->duration;

	// Loook at objects before foating
	AMC::events_t* camevent5 = new AMC::events_t();
	camevent5->start = sequence;
	camevent5->duration = 10.0f;
	camevent5->easingFunction = nullptr;
	camevent5->updateFunction = [this](float t) { this->Cam5(t); };
	events->AddEvent("camevent5", camevent5);

	sequence = camevent5->start + camevent5->duration;

	AMC::events_t* objfloat = new AMC::events_t();
	objfloat->start = sequence;
	objfloat->duration = 6.0f;
	objfloat->easingFunction = nullptr;
	objfloat->updateFunction = [this](float t) { this->ObjectFloat(t); };
	events->AddEvent("floatevent", objfloat);

	sequence = objfloat->start + objfloat->duration;

	AMC::events_t* camevent6 = new AMC::events_t();
	camevent6->start = sequence + 5.0f;
	camevent6->duration = 10.0f;
	camevent6->easingFunction = nullptr;
	camevent6->updateFunction = [this](float t) { this->Cam6(t); };
	events->AddEvent("camevent6", camevent6);

	sequence = camevent6->start + camevent6->duration;

	AMC::events_t* objfall = new AMC::events_t();
	objfall->start = sequence + 2.0f;
	objfall->duration = 2.0f;
	objfall->easingFunction = nullptr;
	objfall->updateFunction = [this](float t) { this->ObjectFall(t); };
	events->AddEvent("fallevent", objfall);

	sequence = objfall->start + objfall->duration;

	AMC::events_t* objreverse = new AMC::events_t();
	objreverse->start = sequence + 7.0f;
	objreverse->duration = 10.0f;
	objreverse->easingFunction = nullptr;
	objreverse->updateFunction = [this](float t) { this->ObjectReverse(t); };
	events->AddEvent("reverseevent", objreverse);

	sequence = objreverse->start + objreverse->duration;

	AMC::events_t* camevent7 = new AMC::events_t();
	camevent7->start = sequence + 5.0f;
	camevent7->duration = 16.0f;
	camevent7->easingFunction = nullptr;
	camevent7->updateFunction = [this](float t) { this->Cam7(t); };
	events->AddEvent("camevent7", camevent7);

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

	AMC::Light point1;
	point1.gpuLight.direction = glm::vec3(-0.50f, 0.7071f, 0.50f); // doesn't matter in case of point lights
	point1.gpuLight.color = glm::vec3(10, 10.0f, 10.0f);
	point1.gpuLight.intensity = 0.5f;
	point1.gpuLight.range = 0.15; // range decides the square fall of distance or attenuation of light
	point1.gpuLight.spotAngle = 1.0f; // for spot lights
	point1.gpuLight.spotExponent = 0.7071f; // for spot lights
	point1.gpuLight.position = glm::vec3(4.250f, 1.40f, -0.70f); // for point and spot lights
	point1.gpuLight.active = 1;
	point1.gpuLight.shadows = false;
	point1.gpuLight.type = AMC::LIGHT_TYPE_POINT;

	AMC::Light point2;
	point2.gpuLight.direction = glm::vec3(0.0f, 0.0f, 0.0f);
	point2.gpuLight.color = glm::vec3(0.0f, 0.0f, 0.0f);
	point2.gpuLight.intensity = 0.5f;
	point2.gpuLight.range = 0.650f;
	point2.gpuLight.spotAngle = 0.0f;
	point2.gpuLight.spotExponent = 45.0f;
	point2.gpuLight.position = glm::vec3(-0.200f, -2.0f, 4.60f);
	point2.gpuLight.active = 1; // need to activate light here
	point2.gpuLight.shadows = true;
	point2.gpuLight.type = AMC::LIGHT_TYPE_POINT;

	AMC::Light point3;
	point3.gpuLight.color = glm::vec3(5.0f, 5.0f, 5.0f);
	point3.gpuLight.intensity = 0.5f;
	point3.gpuLight.range = 0.050f;
	point3.gpuLight.position = glm::vec3(-2.650f, -0.125f, -4.50f);
	point3.gpuLight.active = 1;
	point3.gpuLight.shadows = false;
	point3.gpuLight.type = AMC::LIGHT_TYPE_POINT;

	lightManager->AddLight(point1);
	lightManager->AddLight(point2);
	lightManager->AddLight(point3);
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
float t = 0.0f;
void SuperpositionScene::renderUI()
{
#if defined(_MYDEBUG)
	ImGui::Text("Superposition Scene ");
	ImGui::Text("Scene Time : %0.1f", events->getCurrentTime() + 35.0f);
	switch (AMC::DEBUGMODE) {
	case AMC::MODEL:
		if (ImGui::SliderFloat("Lerp Animation", &t, 0.0f, 1.0f, "%.2f")) {
			models["roomAnimated"].model->lerpAnimation(t);
		}
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
	//models["room"].model->update((float)AMC::deltaTime);
	// models["apple"].matrix = mp->getModelMatrix();
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

	if (key == 'R' || key == 'r') {
		events->resetEvents();
	}
}

void SuperpositionScene::updateRenderContext(AMC::RenderContext& context)
{
	if (!OverrideRenderer)
		return;
	context.IsVGXI = true;
	context.IsGenerateShadowMaps = true;
	context.IsGbuffer = true;
	context.IsDeferredLighting = true;
	context.IsSSAO = true;
	context.IsSkyBox = true;
	context.IsSSR = true;
	context.IsBloom = true;
	context.IsVolumetric = true;
	context.IsToneMap = true;
}

AMC::Camera* SuperpositionScene::getCamera()
{
	return finalCam;
}
