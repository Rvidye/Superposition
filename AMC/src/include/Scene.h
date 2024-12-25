#pragma once

#include<common.h>
#include<Model.h>
#include<Camera.h>
#include<LightManager.h>
#include<VulkanHelperClasses.h>

namespace AMC {

	struct RenderModel {
		Model* model = nullptr;
		glm::mat4 matrix = glm::mat4(1.0f);
		bool visible = true;
		UINT numInstance = 1;
	};

	class Scene {
		public:
			Scene(const AMC::VkContext* vkctx);
			virtual void init() = 0;
			//virtual void render() = 0; I would prefer if we don't let user draw whatever they want and pass system should render all the objects instead.
			virtual void renderDebug() = 0; // only used in debug mode
			virtual void renderUI() = 0;
			virtual void update() = 0;
			virtual void keyboardfunc(char key, UINT keycode) = 0;
			virtual AMC::Camera* getCamera() = 0;

			void BuildTLAS();


			void addModel(const std::string& name, const RenderModel& obj) {
				models[name] = obj;
				reCalculateSceneAABB();
			}

			void removeModel(const std::string& name) {
				models.erase(name);
				reCalculateSceneAABB();
			}
			
			void writeDescSet(int index);

			VkAccelerationStructureKHR getAS() const {
				return tlas;
			}

			static void createDescSetLayout(const AMC::VkContext* ctx, size_t count);
			static void createDescSets();
			static const VkDescSetLayoutManager* vkDescSetLayout() {
				return descSetLayout;
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

			void reCalculateSceneAABB() {
				sceneAABB.mMin = glm::vec3(FLT_MAX);
				sceneAABB.mMax = glm::vec3(-FLT_MAX);
				if (models.empty()) return;
				for (const auto& [name, rendermodel] : models) {

					if (!rendermodel.visible || !rendermodel.model)	continue;

					const AABB& modelAABB = rendermodel.model->aabb;
					const glm::mat4& transform = rendermodel.matrix;

					if (transform == glm::mat4(1.0f)) {
						sceneAABB.mMin = glm::min(sceneAABB.mMin, modelAABB.mMin);
						sceneAABB.mMax = glm::max(sceneAABB.mMax, modelAABB.mMax);
					}
					else {
						glm::vec3 vertices[8] = {
							glm::vec3(modelAABB.mMin.x, modelAABB.mMin.y, modelAABB.mMin.z),
							glm::vec3(modelAABB.mMax.x, modelAABB.mMin.y, modelAABB.mMin.z),
							glm::vec3(modelAABB.mMin.x, modelAABB.mMax.y, modelAABB.mMin.z),
							glm::vec3(modelAABB.mMax.x, modelAABB.mMax.y, modelAABB.mMin.z),
							glm::vec3(modelAABB.mMin.x, modelAABB.mMin.y, modelAABB.mMax.z),
							glm::vec3(modelAABB.mMax.x, modelAABB.mMin.y, modelAABB.mMax.z),
							glm::vec3(modelAABB.mMin.x, modelAABB.mMax.y, modelAABB.mMax.z),
							glm::vec3(modelAABB.mMax.x, modelAABB.mMax.y, modelAABB.mMax.z),
						};

						AABB transformedAABB = { glm::vec3(FLT_MAX), glm::vec3(-FLT_MAX) };
						for (const glm::vec3& vertex : vertices) {
							glm::vec3 worldVertex = glm::vec3(transform * glm::vec4(vertex, 1.0f));
							transformedAABB.mMin = glm::min(transformedAABB.mMin, worldVertex);
							transformedAABB.mMax = glm::max(transformedAABB.mMax, worldVertex);
						}

						// Update scene AABB with transformed AABB.
						sceneAABB.mMin = glm::min(sceneAABB.mMin, transformedAABB.mMin);
						sceneAABB.mMax = glm::max(sceneAABB.mMax, transformedAABB.mMax);
					}
				}
			}

			VkAccelerationStructureKHR tlas;
			bool completed = false;
			std::unordered_map<std::string, RenderModel> models;
			LightManager *lightManager  = nullptr;
			AABB sceneAABB = {glm::vec3(FLT_MAX),glm::vec3(FLT_MAX)};
			const VkContext* ctx;
			VkDescriptorSet descSet;
			static VkDescSetLayoutManager* descSetLayout;
	};
};

