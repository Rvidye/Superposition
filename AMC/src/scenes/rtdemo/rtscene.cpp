#include "rtscene.h"

void rtscene::init() {
	programModel = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\model\\spv\\model.vert.spv"),RESOURCE_PATH("shaders\\model\\spv\\model.frag.spv") });

	AMC::RenderModel cube;
	cube.model = new AMC::Model(RESOURCE_PATH("models\\BoxAnimated.gltf"), aiProcessPreset_TargetRealtime_Quality | aiProcess_FlipUVs | aiProcess_GenBoundingBoxes, ctx);
	cube.matrix = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -2.0f, 0.0f)), glm::vec3(10.0f, 0.5f, 10.0f));
	//addModel("Cube", cube);

	AMC::RenderModel helmet;
	helmet.model = new AMC::Model(RESOURCE_PATH("models\\DamagedHelmet\\DamagedHelmet.gltf"), aiProcessPreset_TargetRealtime_Quality | aiProcess_FlipUVs | aiProcess_GenBoundingBoxes, ctx);
	helmet.matrix = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
	addModel("Helmet", helmet);

	helmet.matrix = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	addModel("Helmet 2", helmet);

	lightManager = new AMC::LightManager();
}


void rtscene::renderDebug() {
}

void rtscene::renderUI() {
}

void rtscene::update() {
}

void rtscene::keyboardfunc(char key, UINT keycode) {
}

AMC::Camera* rtscene::getCamera() {
	return nullptr;
}