#include<common.h>
#include<ModelAssetManager.h>
#include<SceneInstanceManager.h>
#include<RenderExtractor.h>
#include "GBufferPass.h"

void GBufferPass::create(AMC::RenderContext& context)
{
    AMC::MemoryManager mm(ctx);
	m_ProgramGBuffer = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\gbuffer\\gbuffer.vert"), RESOURCE_PATH("shaders\\gbuffer\\gbuffer.frag") });
	m_ProgramGBufferMeshShader = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\gbuffer\\gbuffer.task"), RESOURCE_PATH("shaders\\gbuffer\\gbuffer.mesh"), RESOURCE_PATH("shaders\\gbuffer\\gbuffer.frag") });
	m_ProgramGBufferIndirect = new AMC::ShaderProgram({ RESOURCE_PATH("shaders\\gbuffer\\gbuffer_indirect.task"), RESOURCE_PATH("shaders\\gbuffer\\gbuffer_indirect.mesh"), RESOURCE_PATH("shaders\\gbuffer\\gbuffer_indirect.frag") });

	glCreateFramebuffers(1, &gbuffer);

	glCreateTextures(GL_TEXTURE_2D, 1, &m_textureAlbedoAlpha);
	glTextureParameteri(m_textureAlbedoAlpha, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(m_textureAlbedoAlpha, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(m_textureAlbedoAlpha, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(m_textureAlbedoAlpha, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureStorage2D(m_textureAlbedoAlpha, 1, GL_RGBA8, context.width, context.height);
	glNamedFramebufferTexture(gbuffer, GL_COLOR_ATTACHMENT0, m_textureAlbedoAlpha, 0);

	glCreateTextures(GL_TEXTURE_2D, 1, &m_textureNormal);
	glTextureParameteri(m_textureNormal, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(m_textureNormal, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(m_textureNormal, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(m_textureNormal, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureStorage2D(m_textureNormal, 1, GL_RG8, context.width, context.height);
	glNamedFramebufferTexture(gbuffer, GL_COLOR_ATTACHMENT1, m_textureNormal, 0);

	glCreateTextures(GL_TEXTURE_2D, 1, &m_textureMetallicRoughness);
	glTextureParameteri(m_textureMetallicRoughness, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(m_textureMetallicRoughness, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(m_textureMetallicRoughness, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(m_textureMetallicRoughness, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureStorage2D(m_textureMetallicRoughness, 1, GL_RG8, context.width, context.height);
	glNamedFramebufferTexture(gbuffer, GL_COLOR_ATTACHMENT2, m_textureMetallicRoughness, 0);

	glCreateTextures(GL_TEXTURE_2D, 1, &m_textureEmissive);
	glTextureParameteri(m_textureEmissive, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(m_textureEmissive, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(m_textureEmissive, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(m_textureEmissive, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureStorage2D(m_textureEmissive, 1, GL_R11F_G11F_B10F, context.width, context.height);
	glNamedFramebufferTexture(gbuffer, GL_COLOR_ATTACHMENT3, m_textureEmissive, 0);

	glCreateTextures(GL_TEXTURE_2D, 1, &m_textureVelocity);
	glTextureParameteri(m_textureVelocity, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(m_textureVelocity, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(m_textureVelocity, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(m_textureVelocity, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureStorage2D(m_textureVelocity, 1, GL_RG16F, context.width, context.height);
	glNamedFramebufferTexture(gbuffer, GL_COLOR_ATTACHMENT4, m_textureVelocity, 0);

    m_textureDepth = mm.createImage({ static_cast<uint32_t>(context.width), static_cast<uint32_t>(context.height), 1 }, VK_FORMAT_D32_SFLOAT, VK_IMAGE_VIEW_TYPE_2D, 1, AMC::MemoryFlags::kGlMemoryBit | AMC::MemoryFlags::kVkMemoryBit, VK_IMAGE_USAGE_SAMPLED_BIT);
    glTextureParameteri(m_textureDepth.gl, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(m_textureDepth.gl, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(m_textureDepth.gl, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_textureDepth.gl, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glNamedFramebufferTexture(gbuffer, GL_DEPTH_ATTACHMENT, m_textureDepth.gl, 0);
//
	GLenum drawBuffers[5] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 };
	glNamedFramebufferDrawBuffers(gbuffer, 5, drawBuffers);
	GLenum status = glCheckNamedFramebufferStatus(gbuffer, GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		LOG_ERROR(L"GBuffer framebuffer is not complete! %d", status);
	}

	//save textures to context
	context.textureGBuffer[0] = m_textureAlbedoAlpha;
	context.textureGBuffer[1] = m_textureNormal;
	context.textureGBuffer[2] = m_textureMetallicRoughness;
	context.textureGBuffer[3] = m_textureEmissive;
	context.textureGBufferDepth = m_textureDepth;
	context.textureGBuffer[5] = m_textureVelocity;

	context.gBufferData.AlbedoAlphaTexture = glGetTextureHandleARB(m_textureAlbedoAlpha);
	context.gBufferData.NormalTexture = glGetTextureHandleARB(m_textureNormal);
	context.gBufferData.MetallicRoughnessTexture = glGetTextureHandleARB(m_textureMetallicRoughness);
	context.gBufferData.EmissiveTexture = glGetTextureHandleARB(m_textureEmissive);
	context.gBufferData.DepthTexture = glGetTextureHandleARB(m_textureDepth.gl);
	context.gBufferData.VelocityTexture = glGetTextureHandleARB(m_textureVelocity);

	glMakeTextureHandleResidentARB(context.gBufferData.AlbedoAlphaTexture);
	glMakeTextureHandleResidentARB(context.gBufferData.NormalTexture);
	glMakeTextureHandleResidentARB(context.gBufferData.MetallicRoughnessTexture);
	glMakeTextureHandleResidentARB(context.gBufferData.EmissiveTexture);
	glMakeTextureHandleResidentARB(context.gBufferData.DepthTexture);
	glMakeTextureHandleResidentARB(context.gBufferData.VelocityTexture);

	glCreateBuffers(1, &context.gBufferUBO);
	glNamedBufferData(context.gBufferUBO, sizeof(GBufferDataUBO), &context.gBufferData, GL_STATIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 6, context.gBufferUBO);
}

void GBufferPass::execute(AMC::Scene* scene, AMC::RenderContext& context)
{
	if (!context.IsGbuffer) {
		return;
	}

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glCullFace(GL_BACK);
	glm::vec4 clear_color = glm::vec4(0.0);
	float depth = 1.0f;
	glViewport(0, 0, context.width, context.height);
	glClearNamedFramebufferfv(gbuffer, GL_COLOR, 0, glm::value_ptr(clear_color));
	glClearNamedFramebufferfv(gbuffer, GL_COLOR, 1, glm::value_ptr(clear_color));
	glClearNamedFramebufferfv(gbuffer, GL_COLOR, 2, glm::value_ptr(clear_color));
	glClearNamedFramebufferfv(gbuffer, GL_COLOR, 3, glm::value_ptr(clear_color));
	glClearNamedFramebufferfv(gbuffer, GL_COLOR, 4, glm::value_ptr(clear_color));
	glClearNamedFramebufferfv(gbuffer, GL_DEPTH, 0, &depth);
	glBindFramebuffer(GL_FRAMEBUFFER, gbuffer);

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_CONSERVATIVE_RASTERIZATION_NV);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glCullFace(GL_BACK);
	glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glCullFace(GL_BACK);
	glDepthFunc(GL_LESS);

	// Bind global pooled buffers once for all mesh-shader models
	AMC::ModelAssetManager::Get().BindGlobalBuffers();

	// Bind transform palette and instance SSBOs
	AMC::SceneInstanceManager::Get().BindBuffers();

	// ================================================================
	// Path 1: Indirect multi-draw dispatch for all mesh-shader models
	//
	// A single glMultiDrawMeshTasksIndirectCountNV call replaces the
	// per-model loop for non-skeletal mesh-shader-eligible models.
	// The RenderExtractor has already built the command buffer.
	// ================================================================
	auto& extractor = AMC::RenderExtractor::Get();
	if (context.UseMeshShaders && extractor.HasItems()) {
		m_ProgramGBufferIndirect->use();
		glBindVertexArray(context.emptyVAO);
		extractor.BindBuffers();

		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, extractor.GetIndirectBuffer());
		glBindBuffer(GL_PARAMETER_BUFFER_ARB, extractor.GetCountBuffer());

		glMultiDrawMeshTasksIndirectCountNV(
			0,                                       // indirect buffer offset
			0,                                       // drawcount buffer offset
			extractor.GetCommandCount(),             // max draw count
			sizeof(AMC::GpuMeshTaskCommand)          // stride (16 bytes)
		);

		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
		glBindBuffer(GL_PARAMETER_BUFFER_ARB, 0);
	}

	// ================================================================
	// Path 2: Traditional VAO path for skeletal models and models
	// not eligible for mesh shader dispatch
	// ================================================================
	for (auto& [name, obj] : scene->models) {
		if (!obj.visible)
			continue;

		bool useMeshPath = context.UseMeshShaders
			&& obj.model->hasMeshletData
			&& !(obj.model->haveAnimation && obj.model->animType == AMC::SKELETALANIM);

		// Skip mesh-shader models handled by indirect dispatch above
		if (useMeshPath && obj.model->useGlobalPools) {
			obj.prevMatrix = obj.matrix;
			continue;
		}

		if (useMeshPath) {
			// Fallback: per-model mesh shader for models not in global pools
			m_ProgramGBufferMeshShader->use();
			glBindVertexArray(context.emptyVAO);
			glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(obj.matrix));
			glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(obj.prevMatrix));
			obj.model->drawMeshShader(m_ProgramGBufferMeshShader);
		}
		else {
			m_ProgramGBuffer->use();
			glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(obj.matrix));
			glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(obj.prevMatrix));
			obj.model->draw(m_ProgramGBuffer);
		}
		obj.prevMatrix = obj.matrix;
	}
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

const char* GBufferPass::getName() const
{
	return "G-Buffer Pass";
}

void GBufferPass::renderUI()
{
#ifdef _MYDEBUG
	ImGui::Begin("GBuffer Debug");
	ImGui::Text("Albedo");
	ImGui::Image((void*)(intptr_t)m_textureAlbedoAlpha, ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));
	ImGui::Text("Normal");
	ImGui::Image((void*)(intptr_t)m_textureNormal, ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));
	ImGui::Text("MetallicRough");
	ImGui::Image((void*)(intptr_t)m_textureMetallicRoughness, ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));
	ImGui::Text("Emissive");
	ImGui::Image((void*)(intptr_t)m_textureEmissive, ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));
	ImGui::Text("Velocity");
	ImGui::Image((void*)(intptr_t)m_textureVelocity, ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));
	ImGui::Text("Depth");
	ImGui::Image((void*)(intptr_t)m_textureDepth.gl, ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));
	ImGui::End();
#endif
}
