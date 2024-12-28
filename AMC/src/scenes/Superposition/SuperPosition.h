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
	AMC::SplineCamera* sceneCam, *sceneCam1, *sceneCam2, *sceneCam3, *sceneCam4, *finalCam;
	AMC::SplineCameraAdjuster* camAdjuster;
	AMC::EventManager* events;

	// Defined Callbacks for events
	void Cam1(float);
	void Cam2(float);
	void Cam3(float);
	void Cam4(float);
	void LightRed(float);
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
