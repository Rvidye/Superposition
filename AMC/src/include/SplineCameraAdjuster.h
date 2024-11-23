#pragma once

#include<Camera.h>
#include<BSplineUtils.h>

namespace AMC {

	enum ACTIVESPLINE {
		POSITION,
		FRONT
	};

	class SplineCameraAdjuster {

		private:

			SplineCamera* m_splineCamera;
			SplineAdjuster* m_positionAdjuster;
			SplineAdjuster* m_frontAdjuster;
			UINT m_activespline;

		public:

			SplineCameraAdjuster(SplineCamera* splinecam);
			~SplineCameraAdjuster();
			void render() const;
			void renderUI();
			void keyboardfunc(char keyPressed, UINT keycode);
			SplineCamera* getCamera();
	};
};
