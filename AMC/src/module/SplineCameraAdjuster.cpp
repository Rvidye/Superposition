#include <SplineCameraAdjuster.h>

namespace AMC {
	SplineCameraAdjuster::SplineCameraAdjuster(SplineCamera* splinecam) : m_splineCamera(splinecam) {

		if (!m_splineCamera) {
			LOG_ERROR(L"No Spline Camera Provided");
			return;
		}

		m_positionAdjuster = new AMC::SplineAdjuster(m_splineCamera->m_positionSpline);
		m_frontAdjuster = new AMC::SplineAdjuster(m_splineCamera->m_frontSpline);
		m_activespline = 0;
	}

	SplineCameraAdjuster::~SplineCameraAdjuster(){
		delete m_positionAdjuster;
		delete m_frontAdjuster;
	}

	void SplineCameraAdjuster::render() const {

		if (m_positionAdjuster) {
			m_positionAdjuster->render( glm::vec4(1.0f,0.0f,0.0f,1.0f),
										glm::vec4(0.0f,1.0f,0.0f,1.0f),
										glm::vec4(1.0f,1.0f,0.0f,1.0f));
		}

		if (m_frontAdjuster) {
			m_frontAdjuster->render( glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
									 glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),
									 glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
		}
	}

	void SplineCameraAdjuster::renderUI() {
	#if defined(_MYDEBUG)
		ImGui::Text("Active Spline: %s", m_activespline == ACTIVESPLINE::POSITION ? "Position" : "Front");
		
		if (ImGui::Button("Toggle Active Spline")) {
			if (m_activespline == ACTIVESPLINE::POSITION) {
				m_activespline = ACTIVESPLINE::FRONT;
			}
			else {
				m_activespline = ACTIVESPLINE::POSITION;
			}
		}

		if (m_activespline == ACTIVESPLINE::POSITION) {
			if (m_positionAdjuster) {
				ImGui::Text("Position Spline Adjuster");
				m_positionAdjuster->renderUI();
			}
		}

		if (m_activespline == ACTIVESPLINE::FRONT) {
			if (m_frontAdjuster) {
				ImGui::Text("Front Spline Adjuster");
				m_frontAdjuster->renderUI();
			}
		}
	#endif
	}

	void SplineCameraAdjuster::keyboardfunc(char keyPressed, UINT keycode){

		if (m_activespline == ACTIVESPLINE::POSITION) {
			if (m_positionAdjuster) {
				m_positionAdjuster->keyboardfunc(keyPressed, keycode);
			}
		}

		if (m_activespline == ACTIVESPLINE::FRONT) {
			if (m_frontAdjuster) {
				m_frontAdjuster->keyboardfunc(keyPressed, keycode);
			}
		}
	}

	SplineCamera* SplineCameraAdjuster::getCamera(){
		return m_splineCamera;
	}
}
