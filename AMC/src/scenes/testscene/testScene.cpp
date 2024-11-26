#include "testScene.h"

void testScene::sceneEnd(float t)
{
	if (t > 0.9f)
		completed = true;
}

void testScene::moveModel(float t)
{
	moveZ = std::lerp(-15.0f, -5.0f, t);
}

void testScene::init()
{
	// Shader Program Setup
	programModel = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\model\\model.vert"),RESOURCE_PATH("shaders\\model\\model.frag") });
	programModelAnim = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\model\\modelAnim.vert"),RESOURCE_PATH("shaders\\model\\model.frag") });

	// Models Setup
	modelHelmet = new AMC::Model(RESOURCE_PATH("models\\BoxAnimated.gltf"), aiProcessPreset_TargetRealtime_Quality | aiProcess_FlipUVs | aiProcess_GenBoundingBoxes);
	modelAnim = new AMC::Model(RESOURCE_PATH("models\\CesiumMan\\CesiumMan.gltf"), aiProcessPreset_TargetRealtime_Quality | aiProcess_FlipUVs | aiProcess_GenBoundingBoxes);

	// ModelPlacer
	mp = new AMC::ModelPlacer(glm::vec3(0.0,0.0,-10.0f),glm::vec3(0.0f,0.0f,0.0f),1.0f);

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
}

void testScene::render()
{
	programModel->use();
	glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(AMC::currentCamera->getProjectionMatrix() * AMC::currentCamera->getViewMatrix() * glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, 0.0f, moveZ))));
	modelHelmet->draw(programModel);

	programModel->use();
	glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(AMC::currentCamera->getProjectionMatrix() * AMC::currentCamera->getViewMatrix() * mp->getModelMatrix()));
	modelAnim->draw(programModel);
}

void testScene::renderUI()
{
	ImGui::Text("Tutorial Scene ");
	ImGui::Text("Scene Time : %0.1f", events->getCurrentTime());
	ImGui::Text("Select Debug Mode");
	switch (AMC::DEBUGMODE) {
		case AMC::MODEL:
			mp->renderUI();
		break;
		case AMC::CAMERA:
			camAdjuster->renderUI();
		break;
		case AMC::LIGHT:
			//lm->renderUI();
		break;
		case AMC::SPLINE:
		break;
		case AMC::NONE:
		break;
	}
}

void testScene::update()
{
	events->update();
	modelHelmet->update((float)AMC::deltaTime);
	modelAnim->update((float)AMC::deltaTime);
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
