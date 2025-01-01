#pragma once

#include<Scene.h>
#include<ShaderProgram.h>
#include<VulkanHelperClasses.h>

class rtscene : public AMC::Scene
{
private:
	AMC::ShaderProgram* programModel;
public:
	rtscene(const AMC::VkContext* vkctx) : Scene(vkctx), programModel(nullptr) {}
	void init() override;
	void renderDebug() override;
	void renderUI() override;
	void update() override;
	void keyboardfunc(char key, UINT keycode) override;
	AMC::Camera* getCamera() override;
};

