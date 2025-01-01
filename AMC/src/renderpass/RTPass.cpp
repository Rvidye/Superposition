#include "RTPass.h"

RTPass::RTPass(const AMC::VkContext* ctx) : vkctx(ctx), vkcmdbuffer(new AMC::VkCommandBufferManager(ctx)), vkdescmanager(new AMC::VkDescSetLayoutManager(ctx)) {
	computePipeline = VK_NULL_HANDLE;
	computePipelineLayout = VK_NULL_HANDLE;
	outputImage = {};
}

void RTPass::create(AMC::RenderContext& context) {
	AMC::MemoryManager mm(vkctx);
	outputImage = mm.createImage({ 800, 600, 1 }, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_VIEW_TYPE_2D, 1, AMC::MemoryFlags::kVkMemoryBit, VK_IMAGE_USAGE_STORAGE_BIT);

	vkdescmanager->addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
	vkdescmanager->addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT);
	vkdescmanager->generateDescSetLayout();

	vkcmdbuffer->allocate(kCommandBufferCount);

	std::vector<VkDescriptorSetLayout> descSetLayouts{ AMC::Scene::vkDescSetLayout()->vkDescSetLayout(), vkdescmanager->vkDescSetLayout()};

	VkPipelineLayoutCreateInfo pipelineLayoutCI{};
	pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCI.setLayoutCount = static_cast<uint32_t>(descSetLayouts.size());
	pipelineLayoutCI.pSetLayouts = descSetLayouts.data();
	vkCreatePipelineLayout(vkctx->vkDevice(), &pipelineLayoutCI, nullptr, &computePipelineLayout);

	VkComputePipelineCreateInfo computePipelineCI{};
	computePipelineCI.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCI.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	computePipelineCI.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	computePipelineCI.stage.pName = "main";
	computePipelineCI.stage.module = AMC::loadShaderModule(vkctx, RESOURCE_PATH("shaders/raytracing/spv/vk_rt.comp.spv"));
	computePipelineCI.layout = computePipelineLayout;
	vkCreateComputePipelines(vkctx->vkDevice(), VK_NULL_HANDLE, 1, &computePipelineCI, nullptr, &computePipeline);
	
	VkSamplerCreateInfo samplerCI{};
	samplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

	vkCreateSampler(vkctx->vkDevice(), &samplerCI, nullptr, &sampler);
}

void RTPass::writeDescSet(AMC::RenderContext& context) {
	vkdescmanager->generateDescSet();
	VkDescriptorImageInfo desc{};
	desc.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	desc.imageView = outputImage.view;
	desc.sampler = VK_NULL_HANDLE;
	vkdescmanager->writeToDescSet(0, 0, desc);
	desc.imageView = context.textureGBufferDepth.view;
	desc.sampler = sampler;
	vkdescmanager->writeToDescSet(0, 1, desc);

	vkcmdbuffer->begin();
	VkImageSubresourceRange range{};
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	range.levelCount = 1;
	range.layerCount = 1;
	outputImage.transistionImageLayout(vkcmdbuffer->get(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_NONE, VK_ACCESS_MEMORY_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, range);
	range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	context.textureGBufferDepth.transistionImageLayout(vkcmdbuffer->get(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_NONE, VK_ACCESS_MEMORY_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, range);
	vkcmdbuffer->end();
	vkcmdbuffer->submit();
	vkQueueWaitIdle(vkctx->vkQueue());
}

void RTPass::execute(AMC::Scene* scene, AMC::RenderContext& context) {
	static uint32_t frameIndex = 0;
	uint32_t cmdIndex = frameIndex % RTPass::kCommandBufferCount;

	vkcmdbuffer->begin(cmdIndex);
	VkCommandBuffer cmdBuffer = vkcmdbuffer->get(cmdIndex);
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
	std::vector<VkDescriptorSet> descSets{ scene->descSet, vkdescmanager->vkDescSet(0) };
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, static_cast<uint32_t>(descSets.size()), descSets.data(), 0, nullptr);
	vkCmdDispatch(cmdBuffer, 800 / 8, 600 / 8, 1);

	vkcmdbuffer->end(cmdIndex);

	vkcmdbuffer->submit(cmdIndex);
	vkQueueWaitIdle(vkctx->vkQueue());
	frameIndex++;
}

const char* RTPass::getName() const {
	return "RTPass";
}

void RTPass::renderUI() {

}

RTPass::~RTPass() {
	vkcmdbuffer->free();
}