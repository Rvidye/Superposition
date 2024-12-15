#include "RTPass.h"

RTPass::RTPass(const AMC::VkContext* ctx) : vkctx(ctx), vkcmdbuffer(new AMC::VkCommandBufferManager(ctx)) {
	computePipeline = VK_NULL_HANDLE;
	computePipelineLayout = VK_NULL_HANDLE;
}

void RTPass::create() {
	vkcmdbuffer->allocate(kCommandBufferCount);

	VkPipelineLayoutCreateInfo pipelineLayoutCI{};
	pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	vkCreatePipelineLayout(vkctx->vkDevice(), &pipelineLayoutCI, nullptr, &computePipelineLayout);

	VkComputePipelineCreateInfo computePipelineCI{};
	computePipelineCI.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCI.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	computePipelineCI.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	computePipelineCI.stage.pName = "main";
	computePipelineCI.stage.module = AMC::loadShaderModule(vkctx, RESOURCE_PATH("rt.comp.spv"));
	computePipelineCI.layout = computePipelineLayout;
	vkCreateComputePipelines(vkctx->vkDevice(), VK_NULL_HANDLE, 1, &computePipelineCI, nullptr, &computePipeline);
}

void RTPass::execute(const AMC::Scene* scene) {
	static uint32_t frameIndex = 0;
	uint32_t cmdIndex = frameIndex % RTPass::kCommandBufferCount;

	vkcmdbuffer->begin(cmdIndex);
	VkCommandBuffer cmdBuffer = vkcmdbuffer->get(frameIndex);
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
	vkCmdDispatch(cmdBuffer, 32, 32, 32);

	vkcmdbuffer->end(cmdIndex);

	vkcmdbuffer->submit(cmdIndex);
	frameIndex++;
}

RTPass::~RTPass() {
	vkcmdbuffer->free();
}