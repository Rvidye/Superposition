#pragma once

#include<common.h>

namespace AMC {
	class RenderWindow;

	enum MOUSE_BUTTON {
		MOUSE_BUTTON_LEFT,
		MOUSE_BUTTON_RIGHT,
		MOUSE_BUTTON_MIDDLE,
		MOUSE_BUTTON_NONE
	};

	enum MOUSE_ACTION {
		MOUSE_ACTION_PRESS,
		MOUSE_ACTION_RELEASE,
		MOUSE_ACTION_MOVE
	};

	typedef std::function<void(RenderWindow* window, char key, UINT keycode)> KeyboardCallback;
	typedef std::function<void(RenderWindow* window, int button, int action, int x, int y)> MouseCallback;
	typedef std::function<void(RenderWindow* window, UINT width, UINT height)> ResizeCallback;

	class RenderWindow
	{
		public:
			RenderWindow(HINSTANCE, const std::wstring&,const std::wstring&, UINT, UINT ,BOOL);
			~RenderWindow();

			void InitializeWindow(void);
			void ShutDown(void);
			void SetFullScreen(BOOL);

			BOOL IsClosed(void);
			BOOL GetFullScreen(void);
			UINT GetWindowWidth(void);
			UINT GetWindowHeight(void);
			HWND GetWindowHandle(void);
			FLOAT AspectRatio(void);

			void InitializeGL(void);
			void UninitializeGL(void);

			DOUBLE getTime(void);

			void SetWindowSize(UINT,UINT);
			void SetKeyboardFunc(const KeyboardCallback&);
			void SetMouseFunc(const MouseCallback&);
			void SetResizeFunc(const ResizeCallback&);

			BOOL mClosed;
			HDC mHDC;
		private:
			HWND mWindowHandle;
			HINSTANCE mInstance;
			HGLRC mHGLRC;
			std::wstring mWindowClass;
			std::wstring mWindowTitle;
			UINT mWidth, mHeight;
			BOOL mFullScreen;
			KeyboardCallback mKeyboard;
			MouseCallback mMouse;
			ResizeCallback mResize;
	};
	static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
};
