#pragma once

#include<common.h>
#include<Light.h>

namespace AMC {

	class Scene;
	class ShadowManager {
		public:

			ShadowManager(int maxShadows, int maxPointShadows);

			void createShadowMapForLight(Light& light);
			void removeShadowMapForLight(Light& light);

			void renderShadowMaps(const Scene* scene);
			void renderPointShadowMaps(const Scene* scene);

			GLuint getShadowMapTexture() const { return shadowmap; }
			GLuint getPointShadowCubemap() const { return pointShadowCubemap; }

		private:
			GLuint shadowmap;
			GLuint pointShadowCubemap;
			int maxShadowmaps;
			int maxPointShadowcubemaps;
			int currentShadowmaps;
			int currentPointShadowcubemaps;
	};
};
