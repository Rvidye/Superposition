#pragma once

#include<Camera.h>

namespace AMC {

	class Scene {
		public:
			bool completed = false;
			virtual void init() = 0;
			virtual void render() = 0;
			virtual void renderUI() = 0;
			virtual void update() = 0;
			virtual void keyboardfunc(char key, UINT keycode) = 0;
			virtual AMC::Camera* getCamera() = 0;
	};
};

