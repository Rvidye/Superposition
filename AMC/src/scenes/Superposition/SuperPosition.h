#pragma once

#include<common.h>
#include<ShaderProgram.h>
#include<Model.h>
#include<ModelPlacer.h>
#include<Scene.h>
#include<SplineCameraAdjuster.h>
#include<EventManager.h>

class SuperpositionScene : public AMC::Scene {

private:

	AMC::ModelPlacer* mp;
	AMC::SplineCamera* sceneCam;
	AMC::SplineCameraAdjuster* camAdjuster;
	AMC::EventManager* events;

	// Defined Callbacks for events
	void sceneEnd(float);

public:

	void init() override;
	//void render() override;
	void renderDebug() override;
	void renderUI() override;
	void update() override;
	void keyboardfunc(char key, UINT keycode) override;
	void updateRenderContext(AMC::RenderContext& context) override;
	AMC::Camera* getCamera() override;
};
