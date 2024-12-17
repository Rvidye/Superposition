#include "testScene.h"

void testScene::sceneEnd(float t)
{
	if (t > 0.99f)
		completed = true;
}

void testScene::moveModel(float t)
{
	moveZ = std::lerp(-15.0f, -5.0f, t);
	models["cube"].matrix = glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, 0.0f, moveZ));
}

void testScene::init()
{
	// Shader Program Setup
	programModel = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\model\\spv\\model.vert.spv"),RESOURCE_PATH("shaders\\model\\spv\\model.frag.spv") });
	programModelAnim = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\model\\spv\\modelAnim.vert.spv"),RESOURCE_PATH("shaders\\model\\spv\\model.frag.spv") });

	// ModelPlacer
	mp = new AMC::ModelPlacer(glm::vec3(0.0, 0.0, -10.0f), glm::vec3(0.0f, 0.0f, 0.0f), 1.0f);

	// Models Setup
	AMC::RenderModel cubeobj;
	cubeobj.model = new AMC::Model(RESOURCE_PATH("models\\BoxAnimated.gltf"), aiProcessPreset_TargetRealtime_Quality | aiProcess_FlipUVs | aiProcess_GenBoundingBoxes);
	cubeobj.matrix = glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, 0.0f, -10.0f));
	addModel("cube", cubeobj);

	AMC::RenderModel animman;
	animman.model = new AMC::Model(RESOURCE_PATH("models\\DamagedHelmet\\DamagedHelmet.gltf"), aiProcessPreset_TargetRealtime_Quality | aiProcess_FlipUVs | aiProcess_GenBoundingBoxes);
	animman.matrix = mp->getModelMatrix();
	addModel("man", animman);

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
	AMC::events_t *endEvent = new AMC::events_t();
	endEvent->start = 0.0f;
	endEvent->duration = 10.0f;
	endEvent->easingFunction = nullptr;
	endEvent->updateFunction = [this](float t) { this->sceneEnd(t); }; // Bind the member function using a lambda ! did not think this through so here is an ugly hack !!!
	events->AddEvent("SceneEndEvent", endEvent);

	AMC::events_t* moveEvent = new AMC::events_t();
	moveEvent->start = 0.0f;
	moveEvent->duration = 5.0f;
	moveEvent->easingFunction = nullptr;
	moveEvent->updateFunction = [this](float t) { this->moveModel(t); };
	events->AddEvent("MoveModelEvent", moveEvent);

	lightManager = new AMC::LightManager(3, 3);

	AMC::Light directional;
	directional.direction = glm::vec3(0.50f, -0.7071f, -0.50f);
	directional.color = glm::vec3(1.0f, 1.0f, 1.0f);
	directional.intensity = 1.0f;
	directional.range = -1.0f; // if 0.0 then range is infinite, used in case of point and spot lights
	directional.spotAngle = 1.0f; // for spot lights
	directional.spotExponent = 0.7071f; // for spot lights
	directional.position = glm::vec3(0.0f, 0.0f, 0.0f); // for point and spot lights
	directional.active = 1; // need to activate light here
	directional.shadows = true;
	directional.type = AMC::LIGHT_TYPE_DIRECTIONAL; // need to let shader know what type of light is this

	AMC::Light point;
	point.direction = glm::vec3(-0.50f, 0.7071f, 0.50f); // doesn't matter in case of point lights
	point.color = glm::vec3(1.0f, 1.0f, 1.0f);
	point.intensity = 0.5f;
	point.range = -1.0f; // range decides the square fall of distance or attenuation of light
	point.spotAngle = 1.0f; // for spot lights
	point.spotExponent = 0.7071f; // for spot lights
	point.position = glm::vec3(0.0f, 0.0f, 0.0f); // for point and spot lights
	point.active = 1; // need to activate light here
	point.shadows = true;
	point.type = AMC::LIGHT_TYPE_POINT; // need to let shader know what type of light is this

	AMC::Light spot;
	spot.direction = glm::vec3(0.0f, 0.0f, -1.0f);
	spot.color = glm::vec3(0.0f, 1.0f, 0.0f);
	spot.intensity = 0.5f;
	spot.range = 10.0f;
	spot.spotAngle = glm::cos(glm::radians(0.0f)); // requrie a cos(radians) doing here just saves computatiaon of GPU
	spot.spotExponent = glm::cos(glm::radians(45.0f)); // requrie a cos(radians) doing here just saves computatiaon of GPU
	spot.position = glm::vec3(0.0f, 0.0f, 5.0f); // for point and spot lights
	spot.active = 1; // need to activate light here
	spot.type = AMC::LIGHT_TYPE_SPOT; // need to let shader know what type of light is this

	lightManager->addLight(directional);
	lightManager->addLight(spot);
	lightManager->addLight(point);
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
		case AMC::SHADOW:
			lightManager->getShadowMapManager()->renderUI();
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
	models["cube"].model->update((float)AMC::deltaTime);
	models["man"].model->update((float)AMC::deltaTime);
	models["man"].matrix = mp->getModelMatrix();
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

AMC::Camera* testScene::getCamera()
{
	return sceneCam;
}
