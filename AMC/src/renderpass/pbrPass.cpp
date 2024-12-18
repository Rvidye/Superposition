#include<common.h>
#include<TextureManager.h>
#include"pbrPass.h"

void PBRPass::create() {

    m_programTexturedDraw = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\PBR\\pbr.vert"), RESOURCE_PATH("shaders\\PBR\\pbr.frag") });
}

const int LightType_Directional = 0;
const int LightType_Point = 1;
const int LightType_Spot = 2;


struct Light {
    glm::vec3 direction;
    float range;

    glm::vec3 color;
    float intensity;

    glm::vec3 position;
    float innerConeCos;

    float outerConeCos;
    int type;
};

void PBRPass::execute(const AMC::Scene* scene) {

    m_programTexturedDraw->use();

    Light light = { glm::vec3(1.0f,-1.0f,0.0f),2000,glm::vec3(1.0f,1.0f,1.0f),1.0f,glm::vec3(-100.0f,100.0f,0.0f),0.0f,0.0f,LightType_Directional };
    AMC::TextureManager texManager;

    GLuint brdfTex = texManager.LoadTexture("..\\..\\..\\resources\\textures\\brdf.png");

    std::vector < std::string > difffuseTexFaces;
    difffuseTexFaces.push_back("..\\..\\..\\resources\\textures\\skybox\\right.jpg");
    difffuseTexFaces.push_back("..\\..\\..\\resources\\textures\\skybox\\left.jpg");
    difffuseTexFaces.push_back("..\\..\\..\\resources\\textures\\skybox\\top.jpg");
    difffuseTexFaces.push_back("..\\..\\..\\resources\\textures\\skybox\\bottom.jpg");
    difffuseTexFaces.push_back("..\\..\\..\\resources\\textures\\skybox\\front.jpg");
    difffuseTexFaces.push_back("..\\..\\..\\resources\\textures\\skybox\\back.jpg");


    GLuint difffuseMapTex = texManager.LoadCubeTexture(difffuseTexFaces);
    GLuint specularMapTex = texManager.LoadCubeTexture(difffuseTexFaces);

    glUniform3fv(m_programTexturedDraw->getUniformLocation("lights[0].direction"), 1, glm::value_ptr(light.direction));
    glUniform3fv(m_programTexturedDraw->getUniformLocation("lights[0].color"), 1, glm::value_ptr(light.color));
    glUniform3fv(m_programTexturedDraw->getUniformLocation("lights[0].position"), 1, glm::value_ptr(light.position));
    glUniform1f(m_programTexturedDraw->getUniformLocation("lights[0].range"), light.range);
    glUniform1f(m_programTexturedDraw->getUniformLocation("lights[0].intensity"), light.intensity);
    glUniform1i(m_programTexturedDraw->getUniformLocation("lights[0].type"), light.type);

    glBindTextureUnit(6, brdfTex);
    glBindTextureUnit(7, difffuseMapTex);
    glBindTextureUnit(8, specularMapTex);

    //Camera position
    glUniform3fv(m_programTexturedDraw->getUniformLocation("camPos"), 1, glm::value_ptr(AMC::currentCamera->getViewPosition()));


    for (const auto& [name, obj] : scene->models) {

        if (!obj.visible)
            continue;

        glUniformMatrix4fv(m_programTexturedDraw->getUniformLocation("mMat"), 1, GL_FALSE, glm::value_ptr(obj.matrix));
        glUniformMatrix4fv(m_programTexturedDraw->getUniformLocation("vMat"), 1, GL_FALSE, glm::value_ptr(AMC::currentCamera->getViewMatrix()));
        glUniformMatrix4fv(m_programTexturedDraw->getUniformLocation("pMat"), 1, GL_FALSE, glm::value_ptr(AMC::currentCamera->getProjectionMatrix()));

        obj.model->draw(m_programTexturedDraw);
    }
}
