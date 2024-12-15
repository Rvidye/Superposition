#pragma once

#include<common.h>
#include<Light.h>

namespace AMC {

	class Scene;
	class ShaderProgram;
	class ShadowManager {
		public:

			ShadowManager(int maxShadows, int maxPointShadows);

			void createShadowMapForLight(Light& light);
			void removeShadowMapForLight(Light& light);

			void renderShadowMaps(ShaderProgram* program,const Scene* scene);
			void renderPointShadowMaps(ShaderProgram* program,const Scene* scene);

			void renderUI();

			GLuint getShadowMapTexture() const { return shadowmap; }
			GLuint getPointShadowCubemap() const { return pointShadowCubemap; }

		private:
			GLuint shadowmapFBO = 0;
			GLuint pointShadowmapFBO = 0;
			GLuint shadowmap = 0;
			GLuint pointShadowCubemap = 0;
			int maxShadowmaps;
			int maxPointShadowcubemaps;
			int currentShadowmaps = 0;
			int currentPointShadowcubemaps = 0;
#if defined(_MYDEBUG)
			std::vector<GLuint> debugShadowmapView;
			std::vector<GLuint> debugcubemapFaceViews;
#endif
	};
};
