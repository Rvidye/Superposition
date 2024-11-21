#include<Camera.h>

namespace AMC {

	glm::vec3 CalculateFrontVector(float yaw, float pitch) {
		glm::vec3 front;
		front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		front.y = sin(glm::radians(pitch));
		front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		return glm::normalize(front);
	}

	DebugCamera::DebugCamera()
	{
		width = 1920.0f;
		height = 1080.0f;
	}

	DebugCamera::DebugCamera(float _width, float _height, const glm::vec3& _pos) : width(_width), height(_height)
	{
		position = _pos;
		front = CalculateFrontVector(this->yaw, this->pitch);
	}

	const glm::mat4 DebugCamera::getViewMatrix() const
	{
		return glm::lookAt(position, position + front, up);
	}

	const glm::mat4 DebugCamera::getProjectionMatrix() const
	{
		return glm::perspective(glm::radians(45.0f), width / height, 0.1f, 10000.0f);
	}

	const glm::vec3 DebugCamera::getViewPosition() const
	{
		return position;
	}

	void DebugCamera::keyboard(char key, UINT keycode)
	{
		float velocity = movementSpeed;
		if (key == 'W' || key == 'w')
			position += front * velocity;
		if (key == 'S' || key == 's')
			position -= front * velocity;
		if (key == 'A' || key == 'a')
			position -= glm::normalize(glm::cross(front, up)) * velocity;
		if (key == 'D' || key == 'd')
			position += glm::normalize(glm::cross(front, up)) * velocity;
	}

	void DebugCamera::mouse(int button, int action, int x, int y)
	{
		static float lastX;
		static float lastY;

		if (button == AMC::MOUSE_BUTTON_LEFT)
		{
			if (action == AMC::MOUSE_ACTION_RELEASE)
			{
				lastX = 0.0f;
				lastY = 0.0f;
			}
			else if (action == AMC::MOUSE_ACTION_PRESS)
			{
				lastX = (float)x;
				lastY = (float)y;
			}
			else if (action == AMC::MOUSE_ACTION_MOVE)
			{
				float xoffset = x - lastX;
				float yoffset = lastY - y;

				lastX = (float)x;
				lastY = (float)y;

				xoffset *= mouseSensitivity;
				yoffset *= mouseSensitivity;

				this->yaw += xoffset;
				this->pitch += yoffset;

				if (this->pitch > 89.0f) {
					this->pitch = 89.0f;
				}
				else if (this->pitch < -89.0f) {
					this->pitch = -89.0f;
				}
				this->front = CalculateFrontVector(this->yaw, this->pitch);
			}
		}
	}

	void DebugCamera::resize(float width, float height)
	{
		this->width = width;
		this->height = height;
	}
}
