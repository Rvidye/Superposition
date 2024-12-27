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
	void create() override;
	void execute(const AMC::Scene* scene) override;
	void writeDescSet() override;
	~RTPass();
};

