#pragma once

#include <Scene.h>

namespace AMC {
	// Store Extra Data that can be accesses by all render passes
	struct RenderContext {
		GLsizei width = 4096, height = 4096;
		GLsizei screenWidth, screenHeight;
		//GBuffer
		GLuint textureGBuffer[5]; // albedo, normal, metalroughness, emissive, depth
		GLuint textureDeferredResult = 0;
		GLuint textureSSAOResult = 0;
		GLuint textureSSRResult = 0;
		GLuint textureVolumetricResult = 0;
		GLuint textureBloomResult = 0;
		GLuint textureTonemapResult = 0;
		GLuint textureAtmosphere = 0;
		GLuint fboPostDeferred = 0; // seems like a hack but fuck it
		GLuint emptyVAO = 0;
	};

	class RenderPass {
		public:
			virtual ~RenderPass() = default;
			virtual void create(RenderContext& context) = 0;
			virtual void execute(const Scene* scene, RenderContext &context) = 0;
	};


	class Renderer {

		public:
			static GLsizei width, height;
			static RenderContext context;

			void addPass(RenderPass* pass) {
				passes.push_back(pass);
			}

			void initPasses() {
				for (auto pass : passes) {
					pass->create(context);
				}
			}

			void render(const Scene* scene) {
				for (auto pass : passes) {
					pass->execute(scene, context);
				}
			}

			static void resetFBO() {
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				glViewport(0, 0, width, height);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			}

		private:
			std::vector<RenderPass*> passes;
	};
}
