#include "RTPass.h"

RTPass::RTPass(const AMC::VkContext* ctx) : vkctx(ctx), vkcmdbuffer(new AMC::VkCommandBufferManager(ctx)), vkdescmanager(new AMC::VkDescSetLayoutManager(ctx)) {
	computePipeline = VK_NULL_HANDLE;
	computePipelineLayout = VK_NULL_HANDLE;
}

void RTPass::create() {
	AMC::MemoryManager mm(vkctx);
	outputImage = mm.createImage({ 800, 600, 1 }, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_VIEW_TYPE_2D, 1, AMC::MemoryFlags::kVkMemoryBit, VK_IMAGE_USAGE_STORAGE_BIT);

	vkdescmanager->addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
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
}

void RTPass::writeDescSet() {
	vkdescmanager->generateDescSet();
	VkDescriptorImageInfo desc{};
	desc.imageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	desc.imageView = outputImage.view;
	desc.sampler = VK_NULL_HANDLE;
	vkdescmanager->writeToDescSet(0, 0, desc);
}

void RTPass::execute(const AMC::Scene* scene) {
	static uint32_t frameIndex = 0;
	uint32_t cmdIndex = frameIndex % RTPass::kCommandBufferCount;

	vkcmdbuffer->begin(cmdIndex);
	VkCommandBuffer cmdBuffer = vkcmdbuffer->get(cmdIndex);
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
	std::vector<VkDescriptorSet> descSets{ scene->descSet, vkdescmanager->vkDescSet(0) };
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, descSets.size(), descSets.data(), 0, nullptr);
	vkCmdDispatch(cmdBuffer, 1, 1, 1);

	vkcmdbuffer->end(cmdIndex);

	vkcmdbuffer->submit(cmdIndex);
	vkQueueWaitIdle(vkctx->vkQueue());
	frameIndex++;
}

RTPass::~RTPass() {
	vkcmdbuffer->free();
}