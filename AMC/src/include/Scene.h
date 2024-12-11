#pragma once

#include<common.h>
#include<Model.h>
#include<Camera.h>

namespace AMC {

	struct RenderModel {
		Model* model = nullptr;
		glm::mat4 matrix = glm::mat4(1.0f);
		bool visible = true;
		UINT numInstance = 1;
	};

	class Scene {

		public:
			virtual void init() = 0;
			//virtual void render() = 0; I would prefer if we don't let user draw whatever they want and pass system should render all the objects instead.
			virtual void renderDebug() = 0; // only used in debug mode
			virtual void renderUI() = 0;
			virtual void update() = 0;
			virtual void keyboardfunc(char key, UINT keycode) = 0;
			virtual AMC::Camera* getCamera() = 0;

			void addModel(const std::string& name, const RenderModel& obj) {
				models[name] = obj;
			}

			void removeModel(const std::string& name) {
				models.erase(name);
			}

			//TODO: Remove this, models is already a public variable
			const std::unordered_map<std::string, RenderModel>& getModelList() const {
				return models;
			}

			//Made this function const as it is a simple getter
			const RenderModel* getRenderModel(const std::string& name) const {
				auto it = models.find(name);
				if (it != models.end()) {
					return &it->second;
				}
				return nullptr;
			}

			bool completed = false;
			std::unordered_map<std::string, RenderModel> models;
	};
};

