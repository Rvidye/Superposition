#pragma once

#include<RenderPass.h>
#include<ShaderProgram.h>

class TestPass : public AMC::RenderPass {

	public:
		void create() override;
		void execute(const AMC::Scene* scene) override;
		AMC::ShaderProgram* m_programTexturedDraw;
};
