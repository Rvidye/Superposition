#include<common.h>

// Core Headers
#include<Log.h>
#include<RenderWindow.h>
#include<ShaderProgram.h>
#include<AudioPlayer.h>
#include<TextureManager.h>
#include<Model.h>
#include<Camera.h>
#include<Scene.h>
#include<RenderPass.h>
#include<VulkanHelperClasses.h>
#include<MemoryManager.h>

// Render Passes
#include "renderpass/TestPass/TestPass.h"
#include "renderpass/Shadows/ShadowMapPass.h"
#include "renderpass/GBuffer/GBufferPass.h"
#include "renderpass/DebugPass/DebugDrawPass.h"
#include "renderpass/SSAO/SSAOPass.h"
#include "renderpass/SSR/SSR.h"
#include "renderpass/DeferredLight/DeferredLightPass.h"
#include "renderpass/BlitPass/BlitPass.h"
#include "renderpass/AtmosphericScattering/AtmosphericScatter.h"
#include "renderpass/Skybox/SkyBoxPass.h"
#include "renderpass/Bloom/Bloom.h"
#include "renderpass/Tonemap/Tonemap.h"
#include "renderpass/VXGI/Voxelizer.h"
#include "renderpass/VXGI/ConeTracer.h"
#include "renderpass/VolumetricLighting/VolumetricLighting.h"

// Scenes
#include "scenes/testscene/testScene.h"
#include "scenes/AMCBanner/AMCBanner.h"
#include "scenes/Superposition/SuperPosition.h"

// Libraries
#pragma comment(lib,"glew32.lib")
#pragma comment(lib,"OpenGL32.lib")
#pragma comment(lib,"glu32.lib")
#pragma comment(lib,"OpenAL32.lib")
#pragma	comment(lib,"ktx.lib")
#pragma comment(lib,"volk.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "swresample.lib")
#pragma comment(lib, "swscale.lib")


static AMC::RenderWindow* window;
static AMC::VkContext* vkcontext;
DOUBLE AMC::deltaTime = 0;

BOOL AMC::ANIMATING = FALSE;
BOOL AMC::DEBUGCAM = TRUE;
BOOL AMC::MUTE = FALSE;
UINT AMC::DEBUGMODE = AMC::DEBUGMODES::NONE;
std::vector<std::string> debugModes = { "None", "Camera", "Model", "Light", "Spline", "PostProcess"};
AMC::Camera* AMC::currentCamera;

float AMC::fade = 0.0f;
float AMC::bloom_threshold = 1.0f; // threshold for bloom better if untouched
float AMC::bloom_maxcolor = 2.8f; // intensity of bloom
float AMC::VolumeScattering = 0.758f; // ideal value is between 0.5 ~ 0.9
float AMC::VolumeStength = 0.3f; // 0.5 ~ 1.0

void keyboard(AMC::RenderWindow* , char key, UINT keycode);
void mouse(AMC::RenderWindow*, int button, int action, int x, int y);
void resize(AMC::RenderWindow*, UINT width, UINT height);

void RenderFrame(void);
void Update(void);

void InitRenderPasses(void);
void InitScenes(void);
void playNextScene(void);

//MSAA
GLuint msaaFBO;
GLuint msaaColorBuffer;
GLuint msaaDepthBuffer;

AMC::DebugCamera *gpDebugCamera;
AMC::AudioPlayer *gpAudioPlayer;

std::vector<AMC::Scene*> sceneQueue;
AMC::Scene* currentScene = nullptr;

AMC::Renderer* gpRenderer;
GLsizei AMC::Renderer::width = 0;
GLsizei AMC::Renderer::height = 0;
AMC::RenderContext AMC::Renderer::context;

DOUBLE fps = 0.0;
BOOL bPlayAudio = TRUE;

GLuint perframeUBO;
//GBufferPass* gpass;
//DeferredPass* defferedPass;
//BlitPass* finalpass;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int iCmdShow) {
	WNDCLASSEX wc;
	MSG msg{};
	TCHAR szAppName[] = TEXT("MyWindowClass");
	LOG_INFO(L"Log Start");

	#ifdef _MYDEBUG
		LOG_INFO(L"Running In Debug Mode");
	#else
		LOG_INFO(L"Running In Release Mode");
	#endif

	HRESULT hr = CoInitialize(nullptr);
	if (FAILED(hr))
	{
		LOG_ERROR(L"Failed To Initialize COM");
	}

	ZeroMemory(&wc, sizeof(WNDCLASSEX));
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = AMC::WndProc;   
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszClassName = szAppName;
	wc.lpszMenuName = nullptr;

	if (!RegisterClassEx(&wc))
	{
		LOG_ERROR(L"RegisterClassEx Failed");
	}

	window = new AMC::RenderWindow(hInstance, L"MyWindowClass", L"Raster 2023", 720, 480, FALSE);

	window->InitializeWindow();
	window->InitializeGL();
	if(!glewIsSupported("GL_NV_geometry_shader_passthrough"))
		LOG_ERROR(L"GL_NV_geometry_shader_passthrough Not Supported");
	if (!glewIsSupported("GL_NV_viewport_swizzle"))
		LOG_ERROR(L"GL_NV_viewport_swizzle Not Supported");

	const GLubyte* renderer = glGetString(GL_RENDERER);
	const GLubyte* vendor = glGetString(GL_VENDOR);
	const GLubyte* version = glGetString(GL_VERSION);
	const GLubyte* glslVer = glGetString(GL_SHADING_LANGUAGE_VERSION);
	GLint major = 0, minor = 0;
	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);
	LOG(AMC::LogLevel::LOG_WARNING);
	std::cout << "GPU Renderer   : " << renderer << std::endl;
	std::cout << "Vendor		 : " << vendor << std::endl;
	std::cout << "OpenGL Version : " << version << std::endl;
	std::cout << "GLSL Version   : " << glslVer << std::endl;
	std::cout << "OpenGL Version : " << major << "." << minor << std::endl;
	LOG(AMC::LogLevel::LOG_INFO);

	// Setup Input Handling system
	window->SetKeyboardFunc(keyboard);
	window->SetMouseFunc(mouse);
	window->SetResizeFunc(resize);
	resize(window,720, 480);
	AMC::VkContext::Builder builder;
	vkcontext = builder
		.setAPIVersion(VK_API_VERSION_1_3)
		.setRequiredQueueFlags(VK_QUEUE_COMPUTE_BIT)
		.addDeviceExtension(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME)
		.addDeviceExtension(VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME)
		.build();

	InitRenderPasses();
	InitScenes();

	while (!window->IsClosed()) 
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// Render
		RenderFrame();
		static DOUBLE prevTime = window->getTime();
		DOUBLE current = window->getTime();
		DOUBLE delta = current - prevTime;
		prevTime = current;
		AMC::deltaTime = delta;
		fps = 1.0 / delta;
		// Update
		Update();
	};

	for (auto* scene : sceneQueue) {
		delete scene;
	}
	sceneQueue.clear();

	AMC::TextureManager::UnloadTextures();
	window->ShutDown();
	SAFE_DELETE(window);
	return ((int)msg.wParam);
}

LRESULT CALLBACK AMC::WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
#ifdef _MYDEBUG
	if (ImGui_ImplWin32_WndProcHandler(hwnd, iMsg, wParam, lParam))
		return true;
#endif
	int mouseX;
	int mouseY;
	static int currentbutton = AMC::MOUSE_BUTTON_NONE;
	switch (iMsg)
	{
		case WM_SIZE:
			//window->WindowResize(LOWORD(lParam),HIWORD(lParam));
			resize(window, LOWORD(lParam), HIWORD(lParam));
		break;
		case WM_CHAR:
			keyboard(window, (char)wParam, NULL);
		break;
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
			keyboard(window, NULL, (UINT)wParam);
		break;
		case WM_LBUTTONDOWN:
			mouse(window, AMC::MOUSE_BUTTON_LEFT, AMC::MOUSE_ACTION_PRESS, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			currentbutton = AMC::MOUSE_BUTTON_LEFT;
			break;
		case WM_MBUTTONDOWN:
			mouse(window, AMC::MOUSE_BUTTON_MIDDLE, AMC::MOUSE_ACTION_PRESS, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			currentbutton = AMC::MOUSE_BUTTON_MIDDLE;
			break;
		case WM_RBUTTONDOWN:
			mouse(window, AMC::MOUSE_BUTTON_RIGHT, AMC::MOUSE_ACTION_PRESS, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			currentbutton = AMC::MOUSE_BUTTON_RIGHT;
			break;
		case WM_LBUTTONUP:
			mouse(window, AMC::MOUSE_BUTTON_LEFT, AMC::MOUSE_ACTION_RELEASE, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			currentbutton = AMC::MOUSE_BUTTON_NONE;
			break;
		case WM_MBUTTONUP:
			mouse(window, AMC::MOUSE_BUTTON_MIDDLE, AMC::MOUSE_ACTION_RELEASE, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			currentbutton = AMC::MOUSE_BUTTON_NONE;
			break;
		case WM_RBUTTONUP:
			mouse(window, AMC::MOUSE_BUTTON_RIGHT, AMC::MOUSE_ACTION_RELEASE, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			currentbutton = AMC::MOUSE_BUTTON_NONE;
			break;
		case WM_MOUSEMOVE:
			mouseX = GET_X_LPARAM(lParam);
			mouseY = GET_Y_LPARAM(lParam);
			mouse(window, currentbutton, AMC::MOUSE_ACTION_MOVE, mouseX, mouseY);
		break;
		case WM_CLOSE:
			window->mClosed = TRUE;
			PostQuitMessage(0);
		break;
		case WM_DESTROY:
			PostQuitMessage(0);
		break;
		case WM_QUIT:
			PostQuitMessage(0);
		break;
		default:
		break;
	}
	return DefWindowProc(hwnd, iMsg, wParam, lParam);
}

void keyboard(AMC::RenderWindow*, char key, UINT keycode)
{

	if (gpDebugCamera)
		gpDebugCamera->keyboard(key, keycode);

	if (currentScene)
		currentScene->keyboardfunc(key, keycode);

	switch (key)
	{
		case 'w':
		case 'W':
		break;
		case 'F':
		case 'f':
			window->SetFullScreen(!window->GetFullScreen());
		break;
		default:
		break;
	}

	switch (keycode)
	{
		case VK_SPACE:

			if (bPlayAudio)
			{
				gpAudioPlayer->play();
			}

			AMC::ANIMATING = !AMC::ANIMATING;
			if (AMC::ANIMATING) {
				gpAudioPlayer->resume();
			}
			else
			{
				gpAudioPlayer->pause();
			}
		break;
		case VK_F2:
			AMC::DEBUGCAM = !AMC::DEBUGCAM;
			if (AMC::DEBUGCAM) {
				AMC::currentCamera = gpDebugCamera;
			}
		break;
		case VK_ESCAPE:
			window->mClosed = TRUE;
		break;
		default:
		break;
	}
}

void mouse(AMC::RenderWindow*, int button, int action, int x, int y)
{
#ifdef _MYDEBUG
	if ((!ImGui::GetIO().WantCaptureMouse) && (gpDebugCamera))
		gpDebugCamera->mouse(button, action, x, y);
#else
	gpDebugCamera->mouse(button, action, x, y);
#endif
}

void resize(AMC::RenderWindow*, UINT width, UINT height)
{
	window->SetWindowSize(width, height);
	glViewport(0, 0, (GLsizei)width, (GLsizei)height);

	if (gpDebugCamera) {
		gpDebugCamera->setPerspectiveParameters(45.0f, window->AspectRatio());
	}

	if (AMC::currentCamera) {
		AMC::currentCamera->setPerspectiveParameters(45.0f, window->AspectRatio());
	}
	AMC::Renderer::width = (GLsizei)width;
	AMC::Renderer::height = (GLsizei) height;
	AMC::Renderer::context.screenWidth = (GLsizei)width;
	AMC::Renderer::context.screenHeight = (GLsizei)height;
}

AMC::PerFrameData data = {};
void RenderFrame(void)
{

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (currentScene) {

		if (AMC::DEBUGCAM)
			AMC::currentCamera = gpDebugCamera;
		else
			AMC::currentCamera = currentScene->getCamera();

		if (!AMC::currentCamera) { 
			AMC::currentCamera = gpDebugCamera; // just  in case someone fucks up and getCamera returns null we'll fallback to debugcam
		}
		//AMC::currentCamera->setNearFarPlane();
		data.View = AMC::currentCamera->getViewMatrix();
		data.InvView = glm::inverse(data.View);
		data.Projection = AMC::currentCamera->getProjectionMatrix();
		data.InvProjection = glm::inverse(data.Projection);
		data.ProjView = data.Projection * data.View;
		data.InvProjView = glm::inverse(data.ProjView);
		data.NearPlane = AMC::currentCamera->getNearPlane();
		data.FarPlane = AMC::currentCamera->getFarPlane();
		data.ViewPos = AMC::currentCamera->getViewPosition();

		glNamedBufferSubData(perframeUBO, 0, sizeof(AMC::PerFrameData), &data);

		gpRenderer->render(currentScene);
		//currentScene->render();
	}

#ifdef _MYDEBUG
	//finalpass->execute(currentScene, gpRenderer->context);

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
	ImGui::Begin("Debug Window", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Text("Common Controls:");
	ImGui::Text("F : toggle fullscreen");
	ImGui::Text("%.3lf FPS", fps);

	if (ImGui::Button(AMC::ANIMATING ? "Stop Animation" : "Start Animation")) {
		AMC::ANIMATING = !AMC::ANIMATING;
		if (AMC::ANIMATING) {
			gpAudioPlayer->resume();
		}
		else
		{
			gpAudioPlayer->pause();
		}
	}

	if (ImGui::Button(AMC::MUTE ? "UnMute" : "Mute")) {
		AMC::MUTE = !AMC::MUTE;
		gpAudioPlayer->toggleMute();
	}

	if (ImGui::Button(AMC::DEBUGCAM ? "Disable Debug Camera (F2)" : "Enable Debug Camera (F2)")) {
		AMC::DEBUGCAM = !AMC::DEBUGCAM;
		if (AMC::DEBUGCAM) {
			AMC::currentCamera = gpDebugCamera;
		}
	}

	if (AMC::DEBUGCAM) {
		glm::vec3 pos = gpDebugCamera->getViewPosition();
		ImGui::BulletText("Position: X = %.2f, Y = %.2f, Z = %.2f", pos.x,pos.y,pos.z);
		ImGui::BulletText("Yaw: %.2f", gpDebugCamera->getYAW());
		ImGui::BulletText("Pitch: %.2f", gpDebugCamera->getPITCH());
		ImGui::SliderFloat("Speed", &gpDebugCamera->movementSpeed, 0.1f, 100.0f);
		ImGui::SliderFloat("Sensitivity", &gpDebugCamera->mouseSensitivity, 0.1f, 1.0f);
	}

	ImGui::Text("Renderer Properties");
	ImGui::Checkbox("IsVXGI", &gpRenderer->context.IsVGXI);
	ImGui::Checkbox("IsGenerateShadowMaps", &gpRenderer->context.IsGenerateShadowMaps);
	ImGui::Checkbox("IsSSAO", &gpRenderer->context.IsSSAO);
	ImGui::Checkbox("IsSSR", &gpRenderer->context.IsSSR);
	ImGui::Checkbox("IsSkyBox", &gpRenderer->context.IsSkyBox);
	ImGui::Checkbox("IsBloom", &gpRenderer->context.IsBloom);
	ImGui::Checkbox("IsVolumetric", &gpRenderer->context.IsVolumetric);
	ImGui::Checkbox("IsToneMap", &gpRenderer->context.IsToneMap);
	ImGui::Separator();

	ImGui::Text("Select Debug Mode:");
	if (ImGui::BeginCombo("  ", debugModes[AMC::DEBUGMODE].c_str())) {
		for (int i = 0; i < debugModes.size(); i++) {
			bool isSelected = (i == AMC::DEBUGMODE);
			if (ImGui::Selectable(debugModes[i].c_str(), isSelected)) {
				AMC::DEBUGMODE = i;
			}
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	if (currentScene) currentScene->renderUI();

	ImGui::Separator();
	static int selectedPassIndex = -1;
	const std::vector<AMC::RenderPass*>& passes = gpRenderer->getPasses();

	if (ImGui::BeginCombo("Render Pass",
		(selectedPassIndex >= 0 && selectedPassIndex < passes.size()) ?
		passes[selectedPassIndex]->getName() : "None")) {

		if (ImGui::Selectable("None", selectedPassIndex == -1)) {
			selectedPassIndex = -1;
		}

		for (int i = 0; i < passes.size(); i++) {
			bool isSelected = (i == selectedPassIndex);
			if (ImGui::Selectable(passes[i]->getName(), isSelected)) {
				selectedPassIndex = i;
			}
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	if (selectedPassIndex >= 0 && selectedPassIndex < passes.size()) {
		ImGui::Separator();
		passes[selectedPassIndex]->renderUI();
	}

	ImGui::End();
	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif // _MYDEBUG

	SwapBuffers(window->mHDC);
}

void Update(void)
{
	if (currentScene) {
		if (currentScene->completed) {
			playNextScene();
			return;
		}
		currentScene->update();
	}
}

void InitRenderPasses()
{
	glClearDepth(1.0f);
	glClearColor(0.0, 0.5, 0.5f, 1.0f);
	glDisable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
	//glEnable(GL_MULTISAMPLE);
	//glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	glCreateBuffers(1, &perframeUBO);
	glNamedBufferData(perframeUBO, sizeof(AMC::PerFrameData), nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, perframeUBO);

	gpDebugCamera = new AMC::DebugCamera();
	gpAudioPlayer = new AMC::AudioPlayer();

	gpRenderer = new AMC::Renderer();
	glCreateVertexArrays(1,&AMC::Renderer::context.emptyVAO);

	//gpass = new GBufferPass();
	//defferedPass = new DeferredPass();
	//finalpass = new BlitPass();

	// Add passes here
	gpRenderer->addPass(new AtmosphericScatterer());
	gpRenderer->addPass(new ShadowMapPass());
	gpRenderer->addPass(new Voxelizer());
	gpRenderer->addPass(new GBufferPass());
#ifdef _MYDEBUG
	gpRenderer->addPass(new DebugDrawPass());
#endif
	gpRenderer->addPass(new SSAO());
	gpRenderer->addPass(new ConeTracer());
	gpRenderer->addPass(new DeferredPass());
	gpRenderer->addPass(new SkyBoxPass());
	gpRenderer->addPass(new SSR());
	gpRenderer->addPass(new Bloom());
	gpRenderer->addPass(new Volumetric());
	gpRenderer->addPass(new Tonemap());
	gpRenderer->addPass(new BlitPass());

	//gpRenderer->addPass(new TestPass());

	// Create Resouces for all passes
	gpRenderer->initPasses();
	//finalpass->create(gpRenderer->context);
}

void InitScenes(void)
{
	//sceneQueue.push_back(new testScene());
	//sceneQueue.push_back(new AMCBannerScene());
	sceneQueue.push_back(new SuperpositionScene());

	for (auto* scene : sceneQueue) {
		scene->init();
	}
	playNextScene();

}

void playNextScene(void) {
	static int current = -1;
	current++;
	if (current < sceneQueue.size()) {
		currentScene = sceneQueue[current];
	}

	if (current == sceneQueue.size()) {
		currentScene = NULL; // avoid crash after all scenes are done rendering.
	}
}
