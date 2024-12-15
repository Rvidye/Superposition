#pragma once

#include <Scene.h>

namespace AMC {
	class RenderPass {
		public:
			virtual ~RenderPass() = default;
			virtual void create() = 0;
			virtual void execute(const Scene* scene) = 0;
	};

	class Renderer {

		public:
			static GLsizei width, height;
			void addPass(RenderPass* pass) {
				passes.push_back(pass);
			}

			void initPasses() {
				for (auto pass : passes) {
					pass->create();
				}
			}

			void render(const Scene* scene) {
				for (auto pass : passes) {
					pass->execute(scene);
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
