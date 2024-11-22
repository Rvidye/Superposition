#include<common.h>

// My Headers
#include<Log.h>
#include<RenderWindow.h>
#include<ShaderProgram.h>
#include<Camera.h>
#include<AudioPlayer.h>
#include<TextureManager.h>
#include<Model.h>
#include<EventManager.h>

// Libraries
#pragma comment(lib,"glew32.lib")
#pragma comment(lib,"OpenGL32.lib")
#pragma comment(lib,"glu32.lib")
#pragma comment(lib,"OpenAL32.lib")
#pragma	comment(lib,"ktx.lib")


static AMC::RenderWindow* window;
DOUBLE AMC::deltaTime = 0;

BOOL AMC::ANIMATING = FALSE;
BOOL AMC::DEBUGCAM = TRUE;
BOOL AMC::MUTE = FALSE;
UINT AMC::DEBUGMODE = AMC::DEBUGMODES::NONE;
std::vector<std::string> debugModes = { "None", "Camera", "Model", "Light", "Spline" ,"PostProcess"};
AMC::Camera* AMC::currentCamera;

void keyboard(AMC::RenderWindow* , char key, UINT keycode);
void mouse(AMC::RenderWindow*, int button, int action, int x, int y);
void resize(AMC::RenderWindow*, UINT width, UINT height);

void RenderFrame(void);
void Update(void);

void InitPipeline(void);
void InitGraphics(void);

GLuint vao, vbo, tex1,tex2;
AMC::ShaderProgram* program, *programModel, *programModelAnim;
glm::mat4 pMat;
AMC::Model* modelHelmet,*modelAnim;

//MSAA
GLuint msaaFBO;
GLuint msaaColorBuffer;
GLuint msaaDepthBuffer;

AMC::DebugCamera *gpDebugCamera;
AMC::AudioPlayer *gpAudioPlayer;
AMC::EventManager* gpEventManager;

DOUBLE fps = 0.0;
BOOL bPlayAudio = TRUE;
float moveZ = -15.0f;

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
	wc.style = CS_HREDRAW | CS_VREDRAW;
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

	window = new AMC::RenderWindow(hInstance, L"MyWindowClass", L"Raster 2023", 800, 600, FALSE);

	window->InitializeWindow();
	window->InitializeGL();

	// Setup Input Handling system
	window->SetKeyboardFunc(keyboard);
	window->SetMouseFunc(mouse);
	window->SetResizeFunc(resize);
	resize(window,800, 600);

	InitPipeline();
	InitGraphics();

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
		//SwapBuffers(window->mHDC);
	};

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
	if (gpDebugCamera)
		gpDebugCamera->mouse(button, action, x, y);
}

void resize(AMC::RenderWindow*, UINT width, UINT height)
{
	window->SetWindowSize(width, height);
	glViewport(0, 0, (GLsizei)width, (GLsizei)height);
	pMat = glm::perspective(45.0f, (GLfloat)width / (GLfloat)height, 0.1f, 1000.0f);

	if (gpDebugCamera)
		gpDebugCamera->resize((FLOAT)width, (FLOAT)height);
}

void RenderFrame(void)
{
	// Bind and clear the MSAA framebuffer

	//glBindFramebuffer(GL_FRAMEBUFFER, msaaFBO);
	//glViewport(0,0,2048,2048);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 modelMatrix = glm::mat4(1.0f);
	glm::mat4 view = glm::mat4(1.0f);

	modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -15.0f));
	glm::mat4 mvpMatrix = gpDebugCamera->getProjectionMatrix() * gpDebugCamera->getViewMatrix() * modelMatrix;
	program->use();
	glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(mvpMatrix));
	glBindTextureUnit(0, tex2);
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, 0.0f, moveZ));
	mvpMatrix = gpDebugCamera->getProjectionMatrix() * gpDebugCamera->getViewMatrix() * modelMatrix;
	programModel->use();
	glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(mvpMatrix));
	modelHelmet->draw(programModel);

	modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.0f, -15.0f));
	mvpMatrix = gpDebugCamera->getProjectionMatrix() * gpDebugCamera->getViewMatrix() * modelMatrix;
	programModelAnim->use();
	glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(mvpMatrix));
	modelAnim->draw(programModelAnim);

	// Resolve the MSAA framebuffer to the default framebuffer (screen)
	//glBindFramebuffer(GL_READ_FRAMEBUFFER, msaaFBO);
	//glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // Default framebuffer
	//glBlitFramebuffer(0, 0, 2048, 2048, 0, 0, 2048, 2048, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//glViewport(0, 0, window->GetWindowWidth(), window->GetWindowHeight());

#ifdef _MYDEBUG
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
	}

	if (AMC::DEBUGCAM) {
		glm::vec3 pos = gpDebugCamera->getViewPosition();
		ImGui::BulletText("Position: X = %.2f, Y = %.2f, Z = %.2f", pos.x,pos.y,pos.z);
		ImGui::BulletText("Yaw: %.2f", gpDebugCamera->getYAW());
		ImGui::BulletText("Pitch: %.2f", gpDebugCamera->getPITCH());
		ImGui::SliderFloat("Speed", &gpDebugCamera->movementSpeed, 0.1f, 100.0f);
		ImGui::SliderFloat("Sensitivity", &gpDebugCamera->mouseSensitivity, 0.1f, 1.0f);
	}

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

	ImGui::End();
	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif // _MYDEBUG

	SwapBuffers(window->mHDC);
}

void Update(void)
{
	gpEventManager->update();
	modelHelmet->update((float)AMC::deltaTime);
	modelAnim->update((float)AMC::deltaTime);
}

void InitPipeline()
{
	program = new AMC::ShaderProgram({RESOURCE_PATH("shaders\\test\\basic.vert"),RESOURCE_PATH("shaders\\test\\basic.frag")});

	programModel = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\model\\model.vert"),RESOURCE_PATH("shaders\\model\\model.frag") });
	programModelAnim = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\model\\modelAnim.vert"),RESOURCE_PATH("shaders\\model\\model.frag") });

	gpDebugCamera = new AMC::DebugCamera();
	gpAudioPlayer = new AMC::AudioPlayer();

}

void InitGraphics()
{
	glClearDepth(1.0f);
	glClearColor(0.0, 0.0, 1.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_MULTISAMPLE);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	{
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);

		const GLfloat triangleVertices[] =
		{
			//vertices          // uv
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f
		};

		glBufferData(GL_ARRAY_BUFFER, sizeof(triangleVertices), triangleVertices, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), NULL);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	glBindVertexArray(0);

	tex1 = AMC::TextureManager::LoadTexture(RESOURCE_PATH("textures\\temp.jpg"));
	tex2 = AMC::TextureManager::LoadKTX2Texture(RESOURCE_PATH("textures\\2d_rgba8.ktx2"));

	modelHelmet = new AMC::Model(RESOURCE_PATH("models\\BoxAnimated.gltf"), aiProcessPreset_TargetRealtime_Quality | aiProcess_FlipUVs | aiProcess_GenBoundingBoxes);
	modelAnim = new AMC::Model(RESOURCE_PATH("models\\CesiumMan\\CesiumMan.gltf"), aiProcessPreset_TargetRealtime_Quality | aiProcess_FlipUVs | aiProcess_GenBoundingBoxes);

	gpEventManager = new AMC::EventManager({{"Event1", 0.0f, 5.0f,[](float t) {moveZ = std::lerp(-15.0f,-5.0f,t); }, nullptr}});

	// Setup All FBO's 

	int renderWidth = 2048;  // 4K width
	int renderHeight = 2048; // 4K height
	int sampleCount = 4;     // 4x MSAA

	glCreateFramebuffers(1, &msaaFBO);

	glCreateRenderbuffers(1, &msaaColorBuffer);
	glNamedRenderbufferStorageMultisample(msaaColorBuffer, sampleCount, GL_RGBA8, renderWidth, renderHeight);
	glNamedFramebufferRenderbuffer(msaaFBO, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, msaaColorBuffer);

	glCreateRenderbuffers(1, &msaaDepthBuffer);
	glNamedRenderbufferStorageMultisample(msaaDepthBuffer, sampleCount, GL_DEPTH24_STENCIL8, renderWidth, renderHeight);
	glNamedFramebufferRenderbuffer(msaaFBO, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, msaaDepthBuffer);

	if (glCheckNamedFramebufferStatus(msaaFBO, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "MSAA Framebuffer not complete!" << std::endl;
	}
}

