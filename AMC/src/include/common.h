#pragma once

// Putting All Common Headers Here So Won't Have to include them again and again

// Windows Headers
#include<Windows.h>
#include<windowsx.h>
#include<wrl/client.h>

// C RunTime Header Files
#include<stdlib.h>
#include<malloc.h>
#include<tchar.h>
#include<time.h>
#include<atlbase.h>
#include<atltime.h>

// C++ Headers
#include<iostream>
#include<memory>
#include<string>
#include<stack>
#include<cstring>
#include<ctime>
#include<unordered_map>
#include<filesystem>
#include<functional>
#include<tuple>
#include<vector>
#include<algorithm>
#include<queue>

// GL and GLEW
#include<GL/glew.h>
#include<GL/wglew.h>
#include<GL/gl.h>

// KTX
#include<ktx/ktx.h>

#ifdef _MYDEBUG
// IMGUI
#include<imgui/imgui.h>
#include<imgui/imgui_impl_opengl3.h>
#include<imgui/imgui_impl_win32.h>
// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif // _MYDEBUG

// GLM
#include<glm/glm.hpp>
#include<glm/ext.hpp>

#include<Log.h>

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)       { if (p) { delete (p);     (p) = nullptr; } }
#endif
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if (p) { delete[] (p);   (p) = nullptr; } }
#endif
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p) = nullptr; } }
#endif

#define RESOURCE_PATH(relative) ("..\\..\\..\\resources\\" + std::string(relative))

namespace AMC {
	extern DOUBLE deltaTime;

	extern BOOL ANIMATING;
	extern BOOL DEBUGCAM;
	extern BOOL MUTE;
	extern UINT DEBUGMODE;

	enum DEBUGMODES {
		NONE = 0,
		CAMERA,
		MODEL,
		LIGHT,
		SHADOW,
		SPLINE,
		POSTPROCESS
	};

	class Camera;
	extern Camera* currentCamera;

	struct PerFrameData
	{
		glm::mat4 ProjView;     
		glm::mat4 View;         
		glm::mat4 InvView;      
		glm::vec3 ViewPos;      
		float padding1;         
		glm::mat4 Projection;   
		glm::mat4 InvProjection;
		glm::mat4 InvProjView;  
		float NearPlane;        
		float FarPlane;         
		float padding2;         
		float padding3;         
	};
};

