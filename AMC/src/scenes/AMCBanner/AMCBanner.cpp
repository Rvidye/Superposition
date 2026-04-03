#include <RenderPass.h>
#include<TextureManager.h>
#include "AMCBanner.h"

void AMCBannerScene::sceneStart(float t)
{
	videoPlayer->update(0.1f);
	outputTex = videoPlayer->getTexture();
}

void AMCBannerScene::sceneEnd(float t)
{
	AMC::fade = std::lerp(AMC::fade, 0.0f, t); 
	if (t >= 1.0f)
		completed = true;
}

//void AMCBannerScene::RenderBanner2(float)
//{
//	//outputTex = textureBanner2;
//}
//
//void AMCBannerScene::RenderBanner1(float t)
//{
//	//outputTex = textureBanner1;
//}

void AMCBannerScene::init()
{
	// Shader Program Setup

	// event manager setup

	videoPlayer = new AMC::VideoPlayer(RESOURCE_PATH("textures\\output_video.mp4"));
	videoPlayer->play();

	events = new AMC::EventManager();

	AMC::events_t* videoEvent = new AMC::events_t();
	videoEvent->start = 0.0f;
	videoEvent->duration = 53.0f;
	videoEvent->easingFunction = nullptr;
	videoEvent->updateFunction = [this](float t) { this->sceneStart(t); };
	events->AddEvent("VideoPlayBackEvent", videoEvent);

	AMC::events_t *endEvent = new AMC::events_t();
	endEvent->start = 53.0f;
	endEvent->duration = 1.0f;
	endEvent->easingFunction = nullptr;
	endEvent->updateFunction = [this](float t) { this->sceneEnd(t); };
	events->AddEvent("SceneEndEvent", endEvent);
}

void AMCBannerScene::renderDebug()
{
	switch (AMC::DEBUGMODE) {
	case AMC::MODEL:
		break;
	case AMC::CAMERA:
		break;
	case AMC::LIGHT:
		break;
	case AMC::SPLINE:
		break;
	case AMC::NONE:
		break;
	}
}

void AMCBannerScene::renderUI()
{
#if defined(_MYDEBUG)
	ImGui::Text("AMC Banner Scene ");
	ImGui::Text("Scene Time : %0.1f", events->getCurrentTime());
	switch (AMC::DEBUGMODE) {
	case AMC::MODEL:
		mp->renderUI();
		break;
	case AMC::CAMERA:
		break;
	case AMC::LIGHT:
		//lightManager->renderUI();
		break;
	//case AMC::SHADOW:
	//	lightManager->GetShadowManager()->renderUI();
	//	break;
	case AMC::SPLINE:
		break;
	case AMC::NONE:
		break;
	}
#endif
}

void AMCBannerScene::update()
{
	events->update();
	//models["cube"].model->update((float)AMC::deltaTime);
	//models["man"].model->update((float)AMC::deltaTime);
	//models["man"].matrix = mp->getModelMatrix();
	//reCalculateSceneAABB(); // cannot find better way to do it for now
	//modelAnim->update((float)AMC::deltaTime);
}

void AMCBannerScene::keyboardfunc(char key, UINT keycode)
{
	switch (AMC::DEBUGMODE) {
	case AMC::MODEL:
		mp->keyboardfunc(key);
		break;
	case AMC::CAMERA:
		//camAdjuster->keyboardfunc(key, keycode);
		break;
	case AMC::LIGHT:
		break;
	case AMC::SPLINE:
		break;
	case AMC::NONE:
		break;
	}
}
// callled every fame anyway.
void AMCBannerScene::updateRenderContext(AMC::RenderContext& context)
{
	context.IsVGXI = false;
	context.IsGenerateShadowMaps = false;
	context.IsGbuffer = false;
	context.IsDeferredLighting = false;
	context.IsSSAO = false;
	context.IsSkyBox = false;
	context.IsSSR = false;
	context.IsBloom = false;
	context.IsToneMap = false;
	context.textureTonemapResult = outputTex;
}

AMC::Camera* AMCBannerScene::getCamera()
{
	return nullptr;
}