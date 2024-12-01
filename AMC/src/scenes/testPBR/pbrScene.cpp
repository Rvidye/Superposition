#include "pbrScene.h"

void pbrScene::sceneEnd(float t) {
	if (t > 0.99f)
		completed = true;
}

void pbrScene::moveModel(float t) {
	moveZ = std::lerp(-15.0f, -5.0f, t);
	models["cube"].matrix = glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, 0.0f, moveZ));
}

void pbrScene::init() {
	// Shader Program Setup
	programModel = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\PBR\\pbr.vert"),RESOURCE_PATH("shaders\\PBR\\pbr.frag") });
	//programModelAnim = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\model\\modelAnim.vert"),RESOURCE_PATH("shaders\\model\\model.frag") });

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

	sceneCam = new AMC::SplineCamera(posVec, frontVec);
	camAdjuster = new AMC::SplineCameraAdjuster(sceneCam);

	// event manager setup

	events = new AMC::EventManager();
	AMC::events_t* endEvent = new AMC::events_t();
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

//void pbrScene::render()
//{
//	programModel->use();
//	glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(AMC::currentCamera->getProjectionMatrix() * AMC::currentCamera->getViewMatrix() * glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, 0.0f, moveZ))));
//	modelHelmet->draw(programModel);
//
//	programModel->use();
//	glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(AMC::currentCamera->getProjectionMatrix() * AMC::currentCamera->getViewMatrix() * mp->getModelMatrix()));
//	modelAnim->draw(programModel);
//}

void pbrScene::renderDebug() {
	switch (AMC::DEBUGMODE) {
	case AMC::MODEL:
		break;
	case AMC::CAMERA:
		camAdjuster->render();
		break;
	case AMC::LIGHT:
		break;
	case AMC::SPLINE:
		break;
	case AMC::NONE:
		break;
	}
}

void pbrScene::renderUI() {
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
		//lm->renderUI();
		break;
	case AMC::SPLINE:
		break;
	case AMC::NONE:
		break;
	}
}

void pbrScene::update() {
	events->update();
	models["cube"].model->update((float)AMC::deltaTime);
	models["man"].model->update((float)AMC::deltaTime);
	models["man"].matrix = mp->getModelMatrix();
	//modelAnim->update((float)AMC::deltaTime);
}

void pbrScene::keyboardfunc(char key, UINT keycode) {
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

AMC::Camera* pbrScene::getCamera() {
	return sceneCam;
}
