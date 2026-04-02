#include "RTPass.h"

RTPass::RTPass(const AMC::VkContext* ctx)
	: vkctx(ctx)
	, vkcmdbuffer(new AMC::VkCommandBufferManager(ctx))
	, vkdescmanager(new AMC::VkDescSetLayoutManager(ctx))
	, computePipeline(VK_NULL_HANDLE)
	, computePipelineLayout(VK_NULL_HANDLE)
	, outputImage{}
	, sampler(VK_NULL_HANDLE)
	, vkSemGLReady(VK_NULL_HANDLE)
	, vkSemVKDone(VK_NULL_HANDLE)
	, semGLReadyHandle(nullptr)
	, semVKDoneHandle(nullptr)
	, glSemGLReady(0)
	, glSemVKDone(0)
	, semaphoresValid(false)
{
	for (uint32_t i = 0; i < kCommandBufferCount; i++) {
		frameFences[i] = VK_NULL_HANDLE;
	}
}

bool RTPass::createInteropSemaphores() {

	VkExportSemaphoreCreateInfo exportCI{};
	exportCI.sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO;
	exportCI.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;

	VkSemaphoreCreateInfo semCI{};
	semCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semCI.pNext = &exportCI;

	if (vkCreateSemaphore(vkctx->vkDevice(), &semCI, nullptr, &vkSemGLReady) != VK_SUCCESS)
		return false;
	if (vkCreateSemaphore(vkctx->vkDevice(), &semCI, nullptr, &vkSemVKDone) != VK_SUCCESS)
		return false;

	VkSemaphoreGetWin32HandleInfoKHR getHandleInfo{};
	getHandleInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR;
	getHandleInfo.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;

	getHandleInfo.semaphore = vkSemGLReady;
	if (vkGetSemaphoreWin32HandleKHR(vkctx->vkDevice(), &getHandleInfo, &semGLReadyHandle) != VK_SUCCESS)
		return false;

	getHandleInfo.semaphore = vkSemVKDone;
	if (vkGetSemaphoreWin32HandleKHR(vkctx->vkDevice(), &getHandleInfo, &semVKDoneHandle) != VK_SUCCESS)
		return false;

	glGenSemaphoresEXT(1, &glSemGLReady);
	glGenSemaphoresEXT(1, &glSemVKDone);

	glImportSemaphoreWin32HandleEXT(glSemGLReady, GL_HANDLE_TYPE_OPAQUE_WIN32_EXT, semGLReadyHandle);
	glImportSemaphoreWin32HandleEXT(glSemVKDone, GL_HANDLE_TYPE_OPAQUE_WIN32_EXT, semVKDoneHandle);

	GLboolean isGLReady = glIsSemaphoreEXT(glSemGLReady);
	GLboolean isVKDone = glIsSemaphoreEXT(glSemVKDone);
	if (!isGLReady || !isVKDone) {
		LOG_WARNING(L"RTPass: GL semaphore import failed");
		return false;
	}

	return true;
}

void RTPass::create(AMC::RenderContext& context) {
	context.IsRTShadows = false; // assume disabled until fully validated

	if (!vkctx) {
		LOG_WARNING(L"RTPass: No Vulkan context, RT shadows disabled");
		return;
	}

	AMC::MemoryManager mm(vkctx);

	// Gate 3: Create shared layered output image — one R32F layer per possible shadow-casting light
	outputImage = mm.createImage(
		{ static_cast<uint32_t>(context.width), static_cast<uint32_t>(context.height), 1 },
		VK_FORMAT_R32_SFLOAT, VK_IMAGE_VIEW_TYPE_2D_ARRAY, 1,
		AMC::MemoryFlags::kVkMemoryBit | AMC::MemoryFlags::kGlMemoryBit,
		VK_IMAGE_USAGE_STORAGE_BIT,
		MAX_LIGHTS);

	if (outputImage.gl == 0 || outputImage.vk == VK_NULL_HANDLE) {
		LOG_WARNING(L"RTPass: Failed to create shared RT output image, RT disabled");
		return;
	}

	// Binding 0: depth sampler (combined image sampler)
	// Binding 1: RT output (storage image array)
	vkdescmanager->addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT);
	vkdescmanager->addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
	vkdescmanager->generateDescSetLayout();

	vkcmdbuffer->allocate(kCommandBufferCount);

	std::vector<VkDescriptorSetLayout> descSetLayouts{ AMC::Scene::vkDescSetLayout()->vkDescSetLayout(), vkdescmanager->vkDescSetLayout() };

	VkPushConstantRange pushConstantRange{};
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(RTPushConstants);
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

	if (computePipelineCI.stage.module == VK_NULL_HANDLE) {
		LOG_WARNING(L"RTPass: Failed to load RT shader, RT disabled");
		return;
	}

	vkCreateComputePipelines(vkctx->vkDevice(), VK_NULL_HANDLE, 1, &computePipelineCI, nullptr, &computePipeline);
	vkDestroyShaderModule(vkctx->vkDevice(), computePipelineCI.stage.module, nullptr);

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

	// Create fences for tracking command buffer completion
	VkFenceCreateInfo fenceCI{};
	fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	for (uint32_t i = 0; i < kCommandBufferCount; i++) {
		vkCreateFence(vkctx->vkDevice(), &fenceCI, nullptr, &frameFences[i]);
	}

	semaphoresValid = createInteropSemaphores();
	if (!semaphoresValid) {
		LOG_WARNING(L"RTPass: Semaphore interop failed, RT disabled (no glFinish fallback)");
		return;
	}

	// Create a 2D view of layer 0 for ImGui debug preview (GL_TEXTURE_2D_ARRAY can't be displayed by ImGui)
	glGenTextures(1, &debugTextureView);
	glTextureView(debugTextureView, GL_TEXTURE_2D, outputImage.gl, GL_R32F, 0, 1, 0, 1);
	glTextureParameteri(debugTextureView, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(debugTextureView, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	context.textureRTShadow = outputImage.gl;
	context.IsRTShadows = true;
	LOG_INFO(L"RTPass: RT shadows enabled with semaphore interop");
}

void RTPass::writeDescSet(AMC::RenderContext& context) {
	if (computePipeline == VK_NULL_HANDLE) return;
	vkdescmanager->generateDescSet();

	VkDescriptorImageInfo desc{};
	desc.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	desc.imageView = context.textureGBufferDepth.view;
	desc.sampler = sampler;
	vkdescmanager->writeToDescSet(0, 0, desc);

	desc.imageView = outputImage.view;
	desc.sampler = VK_NULL_HANDLE;
	vkdescmanager->writeToDescSet(0, 1, desc);

	// Transition images to GENERAL layout for compute access
	vkcmdbuffer->begin();
	VkImageSubresourceRange range{};
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	range.levelCount = 1;
	range.layerCount = MAX_LIGHTS;
	outputImage.transistionImageLayout(vkcmdbuffer->get(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_NONE, VK_ACCESS_MEMORY_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, range);
	range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	range.layerCount = 1;
	context.textureGBufferDepth.transistionImageLayout(vkcmdbuffer->get(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_NONE, VK_ACCESS_MEMORY_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, range);
	vkcmdbuffer->end();
	vkcmdbuffer->submit();
	vkQueueWaitIdle(vkctx->vkQueue());
}

void RTPass::glSignalGBufferDone() {
	glSignalSemaphoreEXT(glSemGLReady, 0, nullptr, 0, nullptr, nullptr);
	glFlush();
}

void RTPass::glWaitRTDone() {
	GLenum srcLayout = GL_LAYOUT_GENERAL_EXT;
	glWaitSemaphoreEXT(glSemVKDone, 0, nullptr, 1, &outputImage.gl, &srcLayout);
}

void RTPass::execute(AMC::Scene* scene, AMC::RenderContext& context) {
	if (computePipeline == VK_NULL_HANDLE || !semaphoresValid) return;
	scene->RebuildTLAS();
	memset(context.rtLightLayers, -1, sizeof(context.rtLightLayers));
	context.rtShadowLightCount = 0;

	struct RTLightInfo {
		int lightIndex;
		glm::vec4 posRange;
	};
	std::vector<RTLightInfo> rtLights;

	if (scene->lightManager) {
		for (int i = 0; i < MAX_LIGHTS; i++) {
			AMC::Light* light = scene->lightManager->GetLight(i);
			if (!light || !light->gpuLight.active || !light->gpuLight.shadows) continue;
			if (static_cast<int>(rtLights.size()) >= MAX_LIGHTS) break;

			int layerIdx = static_cast<int>(rtLights.size());
			context.rtLightLayers[i] = layerIdx;
			rtLights.push_back({ i, glm::vec4(light->gpuLight.position, light->gpuLight.range) });
		}
	}
	context.rtShadowLightCount = static_cast<int>(rtLights.size());

	if (rtLights.empty()) return;

	static uint32_t frameIndex = 0;
	uint32_t cmdIndex = frameIndex % RTPass::kCommandBufferCount;

	// Wait for this command buffer's previous submission to complete
	vkWaitForFences(vkctx->vkDevice(), 1, &frameFences[cmdIndex], VK_TRUE, UINT64_MAX);
	vkResetFences(vkctx->vkDevice(), 1, &frameFences[cmdIndex]);

	// GL signals that G-buffer depth is ready for VK to read (semaphore interop)
	glSignalGBufferDone();

	vkcmdbuffer->begin(cmdIndex);
	VkCommandBuffer cmdBuffer = vkcmdbuffer->get(cmdIndex);

	VkImageSubresourceRange depthRange{};
	depthRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	depthRange.levelCount = 1;
	depthRange.layerCount = 1;
	context.textureGBufferDepth.transistionImageLayout(cmdBuffer,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		VK_ACCESS_NONE, VK_ACCESS_SHADER_READ_BIT,
		VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL, depthRange);

	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
	std::vector<VkDescriptorSet> descSets{ scene->descSet, vkdescmanager->vkDescSet(0) };
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, static_cast<uint32_t>(descSets.size()), descSets.data(), 0, nullptr);

	uint32_t dispatchX = (context.width + 7) / 8;
	uint32_t dispatchY = (context.height + 7) / 8;

	// Dispatch once per shadow-casting light, each writing to its own layer
	for (int li = 0; li < static_cast<int>(rtLights.size()); li++) {
		RTPushConstants pc{};
		pc.invViewMat = glm::inverse(AMC::currentCamera->getViewMatrix());
		pc.invProjMat = glm::inverse(AMC::currentCamera->getProjectionMatrix());
		pc.lightPosRange = rtLights[li].posRange;
		pc.layerIndex = li;

		vkCmdPushConstants(cmdBuffer, computePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(RTPushConstants), &pc);
		vkCmdDispatch(cmdBuffer, dispatchX, dispatchY, 1);

		// Barrier between dispatches to avoid write-after-write hazard on the output image
		if (li + 1 < static_cast<int>(rtLights.size())) {
			VkMemoryBarrier memBarrier{};
			memBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			memBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			memBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memBarrier, 0, nullptr, 0, nullptr);
		}
	}

	VkImageSubresourceRange colorRange{};
	colorRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	colorRange.levelCount = 1;
	colorRange.layerCount = MAX_LIGHTS;
	outputImage.transistionImageLayout(cmdBuffer,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_NONE,
		VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL, colorRange);

	vkcmdbuffer->end(cmdIndex);

	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	VkCommandBuffer cmd = vkcmdbuffer->get(cmdIndex);
	submitInfo.pCommandBuffers = &cmd;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &vkSemGLReady;
	submitInfo.pWaitDstStageMask = &waitStage;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &vkSemVKDone;
	vkQueueSubmit(vkctx->vkQueue(), 1, &submitInfo, frameFences[cmdIndex]);

	glWaitRTDone();

	frameIndex++;
}

const char* RTPass::getName() const {
	return "RTPass";
}

void RTPass::renderUI() {
#ifdef _MYDEBUG
	if (debugTextureView)
		ImGui::Image((void*)(intptr_t)debugTextureView, ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));
#endif
}

RTPass::~RTPass() {
	if (vkctx) {
		vkDeviceWaitIdle(vkctx->vkDevice());
		for (uint32_t i = 0; i < kCommandBufferCount; i++) {
			if (frameFences[i] != VK_NULL_HANDLE)
				vkDestroyFence(vkctx->vkDevice(), frameFences[i], nullptr);
		}
		if (vkSemGLReady != VK_NULL_HANDLE)
			vkDestroySemaphore(vkctx->vkDevice(), vkSemGLReady, nullptr);
		if (vkSemVKDone != VK_NULL_HANDLE)
			vkDestroySemaphore(vkctx->vkDevice(), vkSemVKDone, nullptr);
		if (sampler != VK_NULL_HANDLE)
			vkDestroySampler(vkctx->vkDevice(), sampler, nullptr);
		if (computePipeline != VK_NULL_HANDLE)
			vkDestroyPipeline(vkctx->vkDevice(), computePipeline, nullptr);
		if (computePipelineLayout != VK_NULL_HANDLE)
			vkDestroyPipelineLayout(vkctx->vkDevice(), computePipelineLayout, nullptr);
	}
	if (debugTextureView) glDeleteTextures(1, &debugTextureView);
	if (glSemGLReady) glDeleteSemaphoresEXT(1, &glSemGLReady);
	if (glSemVKDone) glDeleteSemaphoresEXT(1, &glSemVKDone);
	vkcmdbuffer->free();
}
