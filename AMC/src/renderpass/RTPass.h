#pragma once

#include<RenderPass.h>
#include<Scene.h>
#include<VulkanHelperClasses.h>
#include<MemoryManager.h>

class RTPass : public AMC::RenderPass {
private:
	static constexpr uint32_t kCommandBufferCount = 3;

	const AMC::VkContext* vkctx;
	AMC::VkCommandBufferManager* vkcmdbuffer;
	AMC::VkDescSetLayoutManager* vkdescmanager;
	AMC::Image outputImage;
	VkPipeline computePipeline;
	VkPipelineLayout computePipelineLayout;
public:
	RTPass(const AMC::VkContext* ctx);
	void create(AMC::RenderContext& context) override;
	void execute(AMC::Scene* scene, AMC::RenderContext& context) override;
	void writeDescSet() override;
	~RTPass();
};

