#include "RTPass.h"

RTPass::RTPass(const AMC::VkContext* ctx) : vkctx(ctx), vkcmdbuffer(new AMC::VkCommandBufferManager(ctx)), vkdescmanager(new AMC::VkDescSetLayoutManager(ctx)) {
	computePipeline = VK_NULL_HANDLE;
	computePipelineLayout = VK_NULL_HANDLE;
	outputImage = {};
}

void RTPass::create(AMC::RenderContext& context) {
	AMC::MemoryManager mm(vkctx);
	outputImage = mm.createImage({ static_cast<uint32_t>(context.width), static_cast<uint32_t>(context.height), 1 }, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_VIEW_TYPE_2D, 1, AMC::MemoryFlags::kVkMemoryBit, VK_IMAGE_USAGE_STORAGE_BIT);

	vkdescmanager->addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT);
	vkdescmanager->addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
	vkdescmanager->generateDescSetLayout();

	vkcmdbuffer->allocate(kCommandBufferCount);

	std::vector<VkDescriptorSetLayout> descSetLayouts{ AMC::Scene::vkDescSetLayout()->vkDescSetLayout(), vkdescmanager->vkDescSetLayout()};

	VkPushConstantRange pushConstantRange{};
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(glm::mat4);
	pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkPipelineLayoutCreateInfo pipelineLayoutCI{};
	pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCI.setLayoutCount = static_cast<uint32_t>(descSetLayouts.size());
	pipelineLayoutCI.pSetLayouts = descSetLayouts.data();
	pipelineLayoutCI.pushConstantRangeCount = 1;
	pipelineLayoutCI.pPushConstantRanges = &pushConstantRange;
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
	samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCI.magFilter = VK_FILTER_NEAREST;
	samplerCI.minLod = 0.0f;
	samplerCI.maxLod = 1.0f;
	samplerCI.minFilter = VK_FILTER_NEAREST;
	samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;

	vkCreateSampler(vkctx->vkDevice(), &samplerCI, nullptr, &sampler);
}

void RTPass::writeDescSet(AMC::RenderContext& context) {
	vkdescmanager->generateDescSet();
	VkDescriptorImageInfo desc{};
	desc.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	desc.imageView = context.textureGBufferDepth.view;
	desc.sampler = sampler;
	vkdescmanager->writeToDescSet(0, 0, desc);
	desc.imageView = outputImage.view;
	desc.sampler = VK_NULL_HANDLE;
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

	glm::mat4 invMat = glm::inverse(AMC::currentCamera->getProjectionMatrix() * AMC::currentCamera->getViewMatrix());

	vkcmdbuffer->begin(cmdIndex);
	VkCommandBuffer cmdBuffer = vkcmdbuffer->get(cmdIndex);
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
	std::vector<VkDescriptorSet> descSets{ scene->descSet, vkdescmanager->vkDescSet(0) };
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, static_cast<uint32_t>(descSets.size()), descSets.data(), 0, nullptr);
	vkCmdPushConstants(cmdBuffer, computePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(glm::mat4), &invMat);
	vkCmdDispatch(cmdBuffer, context.width / 8, context.height / 8, 1);

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