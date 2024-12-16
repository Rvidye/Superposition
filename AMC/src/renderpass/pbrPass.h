#pragma once

#include<RenderPass.h>
#include<ShaderProgram.h>

class PBRPass : public AMC::RenderPass {

public:
    void create() override;
    void execute(const AMC::Scene* scene) override;
    AMC::ShaderProgram* m_programTexturedDraw;
};
