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

		private:
			std::vector<RenderPass*> passes;
	};
}
