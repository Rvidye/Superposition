#include <RenderPass.h>
#include<TextureManager.h>
#include "AMCBanner.h"

void AMCBannerScene::sceneStart(float t)
{
	videoPlayer->update(events->getCurrentTime());
	outputTex = videoPlayer->getTexture();
}

void AMCBannerScene::sceneEnd(float t)
{
	if (t >= 1.0f)
		completed = true;
}

void AMCBannerScene::RenderBanner2(float)
{
	//outputTex = textureBanner2;
}

void AMCBannerScene::RenderBanner1(float t)
{
	//outputTex = textureBanner1;
}

void AMCBannerScene::init()
{
	// Shader Program Setup

	// event manager setup

	videoPlayer = new AMC::VideoPlayer(RESOURCE_PATH("textures\\AMC.mp4"));
	videoPlayer->play();

	textureBanner1 = AMC::TextureManager::LoadTexture(RESOURCE_PATH("textures\\GL-2.png"));
	textureBanner2 = AMC::TextureManager::LoadTexture(RESOURCE_PATH("textures\\temp.jpg"));

	events = new AMC::EventManager();

	AMC::events_t* videoEvent = new AMC::events_t();
	videoEvent->start = 0.0f;
	videoEvent->duration = videoPlayer->getDuration();
	videoEvent->easingFunction = nullptr;
	videoEvent->updateFunction = [this](float t) { this->sceneStart(t); };
	events->AddEvent("VideoPlayBackEvent", videoEvent);

	AMC::events_t *endEvent = new AMC::events_t();
	endEvent->start = 0.0f;
	endEvent->duration = videoPlayer->getDuration() + 10.0f;
	endEvent->easingFunction = nullptr;
	endEvent->updateFunction = [this](float t) { this->sceneEnd(t); }; // Bind the member function using a lambda ! did not think this through so here is an ugly hack !!!
	events->AddEvent("SceneEndEvent", endEvent);

	AMC::events_t* event1 = new AMC::events_t();
	event1->start = videoPlayer->getDuration();
	event1->duration = 1.0f;
	event1->easingFunction = nullptr;
	event1->updateFunction = [this](float t) { this->RenderBanner1(t); };
	events->AddEvent("BannerRender1", event1);

	AMC::events_t* event2 = new AMC::events_t();
	event2->start = videoPlayer->getDuration() + 2.0f;
	event2->duration = 1.0f;
	event2->easingFunction = nullptr;
	event2->updateFunction = [this](float t) { this->RenderBanner2(t); };
	events->AddEvent("BannerRender2", event2);
}

void AMCBannerScene::renderDebug()
{
	switch (AMC::DEBUGMODE) {
	case AMC::MODEL:
		break;
	case AMC::CAMERA:
		break;
	case AMC::LIGHT:
		lightManager->drawLights();
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
		lightManager->renderUI();
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
	//
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