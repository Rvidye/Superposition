#pragma once

#include<common.h>
#include<RenderWindow.h>

const float YAW = -90.0;
const float PITCH = 0.0;
const float SPEED = 0.5f;
const float SENSITIVITY = 0.2f;
const float ZOOM = 45.0;

namespace AMC 
{
	class Camera 
	{
		public:
			Camera() {};
			virtual ~Camera() {};

			virtual const glm::mat4 getViewMatrix() const = 0;
			virtual const glm::mat4 getProjectionMatrix() const = 0;
			virtual const glm::vec3 getViewPosition() const = 0;
			virtual void keyboard(char key, UINT keycode) = 0;

		protected:
			glm::vec3 position = glm::vec3(0.0f,0.0f,0.0f);
			glm::vec3 front = glm::vec3(0.0f,0.0f,-1.0f);
			glm::vec3 up = glm::vec3(0.0f,1.0f,0.0f);

			glm::mat4 viewMatrix;
			glm::mat4 projectionMatrix;
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
			void keyboard(char key, UINT keycode);
			void mouse(int button, int action, int x, int y);
			void resize(float width, float height);

			FLOAT yaw = YAW;
			FLOAT pitch = PITCH;
			FLOAT width, height;
			FLOAT movementSpeed = SPEED;
			FLOAT mouseSensitivity = SENSITIVITY;
			FLOAT zoom = ZOOM;
	};

}