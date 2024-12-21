#pragma once

#include<common.h>
#include<RenderWindow.h>
//#include<BSpline.h>

const float YAW = -90.0;
const float PITCH = 0.0;
const float SPEED = 0.5f;
const float SENSITIVITY = 0.2f;
const float ZOOM = 45.0;

namespace AMC 
{
	class BsplineInterpolator;
	class Camera 
	{
		public:
			Camera() {};
			virtual ~Camera() {};

			virtual const glm::mat4 getViewMatrix() const = 0;
			virtual const glm::mat4 getProjectionMatrix() const = 0;
			virtual const glm::vec3 getViewPosition() const = 0;
			virtual const float getNearPlane() const = 0;
			virtual const float getFarPlane() const = 0;
			virtual void keyboard(char key, UINT keycode) = 0;
			virtual void setPerspectiveParameters(float fov, float aspectRatio) = 0;
			virtual void setNearFarPlane(float n, float f) = 0;

		protected:

			float fov = 45.0f, aspectRatio = 1920.0f / 1080.0f;
			float nearPlane = 0.1f, farPlane = 1000.0f;
			glm::vec3 position = glm::vec3(0.0f,0.0f,0.0f);
			glm::vec3 front = glm::vec3(0.0f,0.0f,-1.0f);
			glm::vec3 up = glm::vec3(0.0f,1.0f,0.0f);

			glm::mat4 viewMatrix = glm::mat4(1.0f);
			glm::mat4 projectionMatrix = glm::mat4(1.0f);
	};

	class DebugCamera : public Camera {

		public:
			DebugCamera();
			DebugCamera(float width, float height, const glm::vec3& pos);
			FLOAT getYAW() { return yaw; }
			FLOAT getPITCH() { return pitch; }

			const glm::mat4 getViewMatrix() const;
			const glm::mat4 getProjectionMatrix() const;
			const glm::vec3 getViewPosition() const;
			const float getNearPlane() const;
			const float getFarPlane() const;
			void setPerspectiveParameters(float fov, float aspectRatio);
			void setNearFarPlane(float n, float f) { nearPlane = n; farPlane = f; };
			void keyboard(char key, UINT keycode);
			void mouse(int button, int action, int x, int y);
			//void resize(float width, float height);

			FLOAT yaw = YAW;
			FLOAT pitch = PITCH;
			//FLOAT width, height;
			FLOAT movementSpeed = SPEED;
			FLOAT mouseSensitivity = SENSITIVITY;
			FLOAT zoom = ZOOM;
	};

	class SplineCamera : public Camera {
		public:
			SplineCamera(const std::vector<glm::vec3>& positionVector, const std::vector<glm::vec3>& frontVector);
			~SplineCamera();
			void update(float dt);
			const glm::mat4 getViewMatrix() const;
			const glm::mat4 getProjectionMatrix() const;
			const glm::vec3 getViewPosition() const;
			const float getNearPlane() const;
			const float getFarPlane() const;
			void keyboard(char key, UINT keycode);
			void setPerspectiveParameters(float fov, float aspectRatio);
			void setNearFarPlane(float n, float f) { nearPlane = n; farPlane = f; };

		private:
			BsplineInterpolator* m_positionSpline;
			BsplineInterpolator* m_frontSpline;
			float t;
			friend class SplineCameraAdjuster;
	};
}