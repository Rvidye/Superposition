#pragma once

#include<RenderPass.h>
#include<Scene.h>
#include<VulkanHelperClasses.h>
#include<MemoryManager.h>
#include <Light.h>

struct RTPushConstants {
	glm::mat4 invViewMat;
	glm::mat4 invProjMat;
	glm::vec4 lightPosRange; // xyz = position, w = range
	int layerIndex;
	int _pad[3];
};

class RTPass : public AMC::RenderPass {
private:
	static constexpr uint32_t kCommandBufferCount = 3;

	const AMC::VkContext* vkctx;
	AMC::VkCommandBufferManager* vkcmdbuffer;
	AMC::VkDescSetLayoutManager* vkdescmanager;
	AMC::Image outputImage; // image2DArray: one layer per shadow-casting light
	GLuint debugTextureView = 0;
	VkPipeline computePipeline;
	VkPipelineLayout computePipelineLayout;
	VkSampler sampler;
	VkFence frameFences[kCommandBufferCount];

	// GL/VK interop semaphores
	VkSemaphore vkSemGLReady;   // GL signals after G-buffer, VK waits before compute
	VkSemaphore vkSemVKDone;    // VK signals after compute, GL waits before deferred
	HANDLE semGLReadyHandle;
	HANDLE semVKDoneHandle;
	GLuint glSemGLReady;        // GL semaphore object
	GLuint glSemVKDone;         // GL semaphore object
	bool semaphoresValid;

	bool createInteropSemaphores();
public:
	RTPass(const AMC::VkContext* ctx);
	void create(AMC::RenderContext& context) override;
	void execute(AMC::Scene* scene, AMC::RenderContext& context) override;
	void writeDescSet(AMC::RenderContext& context) override;
	const char* getName() const override;
	void renderUI() override;

	// Called by the rendering loop around GL passes
	void glSignalGBufferDone();
	void glWaitRTDone();

	~RTPass();
};
