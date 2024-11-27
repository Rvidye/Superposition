#pragma once

#include<common.h>
#include<ShaderProgram.h>
#include<Model.h>
#include<ModelPlacer.h>
#include<Scene.h>
#include<SplineCameraAdjuster.h>
#include<EventManager.h>

class testScene : public AMC::Scene {

	private:

		AMC::ShaderProgram *programModel, *programModelAnim;
		AMC::Model *modelHelmet, *modelAnim;
		AMC::ModelPlacer* mp;
		AMC::SplineCamera* sceneCam;
		AMC::SplineCameraAdjuster* camAdjuster;
		AMC::EventManager* events;

		// Defined Callbacks for events

		float moveZ = -15.0f;

		void sceneEnd(float);
		void moveModel(float);

	public:

		void init() override;
		//void render() override;
		void renderDebug() override;
		void renderUI() override;
		void update() override;
		void keyboardfunc(char key, UINT keycode) override;
		AMC::Camera* getCamera() override;
};

