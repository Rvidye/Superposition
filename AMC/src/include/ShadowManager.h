#pragma once

#include<common.h>
#include<Light.h>

namespace AMC {

	class Scene;
	class ShaderProgram;
	class ShadowManager {
		public:

			ShadowManager();
			~ShadowManager();

			bool AddShadow(int lightIndex, Light& light);
			void DeleteShadow(int shadowIndex);

			void UpdateShadows(const std::vector<Light>& lights);
			void UpdateShadowUBO();
			void RenderShadowMaps(ShaderProgram* program,const Scene* scene);
			void UpdateShadowLightIndex(int shadowIndex, int newLightIndex);
			void renderUI();

			void BindUBO();

			//GLuint getShadowMapTexture() const { return shadowmap; }
			//GLuint getPointShadowCubemap() const { return pointShadowCubemap; }

			std::vector<Shadow> shadows;
			GLuint shadowsUBO = 0;
	};
};
