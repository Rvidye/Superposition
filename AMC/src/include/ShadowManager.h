#pragma once

#include<common.h>
#include<Light.h>

namespace AMC {

	class RenderExtractor;
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

			// Legacy draw path
			void RenderShadowMaps(ShaderProgram* program, const Scene* scene);

			// Mesh-pipeline path: renders per-face with meshlet culling.
			// Falls back to legacy for skeletal/non-meshlet assets.
			void RenderShadowMapsMesh(
				ShaderProgram* meshProgram,
				ShaderProgram* legacyProgram,
				ShaderProgram* hairProgram,
				const Scene* scene,
				const RenderExtractor& extractor,
				bool useMeshShaders,
				bool renderHair);

			// Mark all shadows as needing update (e.g. when geometry changes)
			void InvalidateAllShadows();

			void UpdateShadowLightIndex(int shadowIndex, int newLightIndex);
			void renderUI();

			void BindUBO();

			std::vector<Shadow> shadows;
			GLuint shadowsUBO = 0;

		private:
			// Per-face FBO for mesh-shader shadow rendering
			GLuint m_meshShadowFBO = 0;
			bool m_meshFBOCreated = false;
	};
};
