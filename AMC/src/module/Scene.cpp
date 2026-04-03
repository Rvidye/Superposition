#include<Scene.h>
#include<VulkanHelperClasses.h>
#include<MemoryManager.h>

namespace AMC {
	VkDescSetLayoutManager* Scene::descSetLayout = nullptr;

	void Scene::createDescSetLayout(const AMC::VkContext* ctx, size_t count) {
		descSetLayout = new VkDescSetLayoutManager(ctx, static_cast<uint32_t>(count));
		descSetLayout->addBinding(0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_COMPUTE_BIT);
		descSetLayout->generateDescSetLayout();
	}

	void Scene::createDescSets() {
		descSetLayout->generateDescSet();
	}

	void Scene::writeDescSet(int index) {
		descSet = descSetLayout->vkDescSet(index);
		// Only write the AS descriptor if we have a valid TLAS.
		// Writing VK_NULL_HANDLE without nullDescriptor causes validation errors.
		if (tlas != VK_NULL_HANDLE) {
			descSetLayout->writeToDescSet(index, 0, tlas);
		}
	}

	Scene::Scene(const AMC::VkContext* vkctx) {
		ctx = vkctx;
		tlas = VK_NULL_HANDLE;
		descSet = VK_NULL_HANDLE;
	}

	void Scene::destroyTLASResources() {
		if (ctx == nullptr) return;
		VkDevice dev = ctx->vkDevice();

		// Wait for any in-flight TLAS work
		if (tlasFence != VK_NULL_HANDLE) {
			vkWaitForFences(dev, 1, &tlasFence, VK_TRUE, UINT64_MAX);
		}

		if (tlas != VK_NULL_HANDLE) {
			vkDestroyAccelerationStructureKHR(dev, tlas, nullptr);
			tlas = VK_NULL_HANDLE;
		}
		// Release persistent buffers
		if (tlasInstanceBuffer.vk != VK_NULL_HANDLE) {
			vkDestroyBuffer(dev, tlasInstanceBuffer.vk, nullptr);
			vkFreeMemory(dev, tlasInstanceBuffer.vkmem, nullptr);
			tlasInstanceBuffer = {};
		}
		if (tlasScratchBuffer.vk != VK_NULL_HANDLE) {
			vkDestroyBuffer(dev, tlasScratchBuffer.vk, nullptr);
			vkFreeMemory(dev, tlasScratchBuffer.vkmem, nullptr);
			tlasScratchBuffer = {};
		}
		if (tlasStorageBuffer.vk != VK_NULL_HANDLE) {
			vkDestroyBuffer(dev, tlasStorageBuffer.vk, nullptr);
			vkFreeMemory(dev, tlasStorageBuffer.vkmem, nullptr);
			tlasStorageBuffer = {};
		}
		tlasMaxInstances = 0;
		tlasScratchSize = 0;
		tlasUpdateScratchSize = 0;
		tlasStorageSize = 0;
		tlasBuilt = false;
	}

	void Scene::BuildTLAS() {
		if (ctx == nullptr) {
			return;
		}
		MemoryManager* mem = new MemoryManager(ctx);

		std::vector<VkAccelerationStructureInstanceKHR> instances{};
		VkAccelerationStructureDeviceAddressInfoKHR asDeviceAI{};
		asDeviceAI.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		for (const auto& [_, rendermodel] : models) {
			if (!rendermodel.model || !rendermodel.visible) continue;
			for (const auto& mesh : rendermodel.model->meshes) {
				if (mesh->blas == VK_NULL_HANDLE) continue;
				// Skip animated/skinned meshes (Phase 5)
				if (rendermodel.model->haveAnimation) continue;

				asDeviceAI.accelerationStructure = mesh->blas;

				VkAccelerationStructureInstanceKHR inst{};
				inst.mask = 0xff;
				inst.accelerationStructureReference = vkGetAccelerationStructureDeviceAddressKHR(ctx->vkDevice(), &asDeviceAI);
				glm::mat4 finalTransform = rendermodel.matrix * mesh->nodeTransform;
				glm::mat3x4 vkTransform = glm::make_mat3x4(&glm::transpose(finalTransform)[0][0]);
				std::copy_n(&vkTransform[0][0], 3 * 4, &inst.transform.matrix[0][0]);
				instances.push_back(inst);
			}
		}

		if (instances.empty()) {
			tlasBuilt = false;
			delete mem;
			return;
		}

		uint32_t primCount = static_cast<uint32_t>(instances.size());

		// Allocate instance buffer
		tlasInstanceBuffer = mem->createBuffer(
			sizeof(VkAccelerationStructureInstanceKHR) * primCount,
			AMC::MemoryFlags::kVkMemoryBit,
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, true);
		tlasInstanceBuffer.copyFromCpu(ctx, instances, 0);
		tlasMaxInstances = primCount;

		VkAccelerationStructureGeometryKHR asGeometry{};
		asGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		asGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		asGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		asGeometry.geometry.instances.data.deviceAddress = tlasInstanceBuffer.deviceAddress;
		asGeometry.geometry.instances.arrayOfPointers = VK_FALSE;

		VkAccelerationStructureBuildGeometryInfoKHR asBuildGeomInfo{};
		asBuildGeomInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		asBuildGeomInfo.geometryCount = 1;
		asBuildGeomInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		asBuildGeomInfo.pGeometries = &asGeometry;
		asBuildGeomInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		asBuildGeomInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR
			| VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;

		VkAccelerationStructureBuildSizesInfoKHR sizeInfo{};
		sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
		vkGetAccelerationStructureBuildSizesKHR(ctx->vkDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &asBuildGeomInfo, &primCount, &sizeInfo);

		tlasScratchSize = sizeInfo.buildScratchSize;
		tlasUpdateScratchSize = sizeInfo.updateScratchSize;
		tlasStorageSize = sizeInfo.accelerationStructureSize;

		VkDeviceSize maxScratch = std::max(tlasScratchSize, tlasUpdateScratchSize);
		tlasScratchBuffer = mem->createBuffer(maxScratch, AMC::MemoryFlags::kVkMemoryBit, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, true, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		asBuildGeomInfo.scratchData.deviceAddress = tlasScratchBuffer.deviceAddress;

		tlasStorageBuffer = mem->createBuffer(tlasStorageSize, AMC::MemoryFlags::kVkMemoryBit, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, false, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		VkAccelerationStructureCreateInfoKHR asCI{};
		asCI.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		asCI.size = tlasStorageSize;
		asCI.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		asCI.buffer = tlasStorageBuffer.vk;
		vkCreateAccelerationStructureKHR(ctx->vkDevice(), &asCI, nullptr, &tlas);

		asBuildGeomInfo.dstAccelerationStructure = tlas;

		VkCommandBufferManager cmdBufferManager(ctx);
		cmdBufferManager.allocate(1);
		cmdBufferManager.begin();

		VkAccelerationStructureBuildRangeInfoKHR buildrangeAS{};
		buildrangeAS.primitiveCount = primCount;
		std::vector<VkAccelerationStructureBuildRangeInfoKHR*> buildRange{ &buildrangeAS };
		vkCmdBuildAccelerationStructuresKHR(cmdBufferManager.get(), 1, &asBuildGeomInfo, buildRange.data());

		cmdBufferManager.end();
		cmdBufferManager.submit();
		vkQueueWaitIdle(ctx->vkQueue()); // OK for one-time build

		// Create a reusable fence for per-frame TLAS updates
		if (tlasFence == VK_NULL_HANDLE) {
			VkFenceCreateInfo fci{};
			fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
			vkCreateFence(ctx->vkDevice(), &fci, nullptr, &tlasFence);
		}

		tlasBuilt = true;
		delete mem;
	}

	void Scene::RebuildTLAS() {
		if (ctx == nullptr || !tlasBuilt || tlas == VK_NULL_HANDLE) {
			return;
		}

		// Wait for previous TLAS update to finish before reusing buffers
		if (tlasFence != VK_NULL_HANDLE) {
			vkWaitForFences(ctx->vkDevice(), 1, &tlasFence, VK_TRUE, UINT64_MAX);
			vkResetFences(ctx->vkDevice(), 1, &tlasFence);
		}

		// Collect current instances with updated transforms
		std::vector<VkAccelerationStructureInstanceKHR> instances{};
		VkAccelerationStructureDeviceAddressInfoKHR asDeviceAI{};
		asDeviceAI.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;

		for (const auto& [_, rendermodel] : models) {
			if (!rendermodel.model || !rendermodel.visible) continue;
			for (const auto& mesh : rendermodel.model->meshes) {
				if (mesh->blas == VK_NULL_HANDLE) continue;
				if (rendermodel.model->haveAnimation) continue;

				asDeviceAI.accelerationStructure = mesh->blas;

				VkAccelerationStructureInstanceKHR inst{};
				inst.mask = 0xff;
				inst.accelerationStructureReference = vkGetAccelerationStructureDeviceAddressKHR(ctx->vkDevice(), &asDeviceAI);
				glm::mat4 finalTransform = rendermodel.matrix * mesh->nodeTransform;
				glm::mat3x4 vkTransform = glm::make_mat3x4(&glm::transpose(finalTransform)[0][0]);
				std::copy_n(&vkTransform[0][0], 3 * 4, &inst.transform.matrix[0][0]);
				instances.push_back(inst);
			}
		}

		if (instances.empty()) return;

		uint32_t primCount = static_cast<uint32_t>(instances.size());

		// If instance count changed, we need a full rebuild (can't update with different count)
		if (primCount != static_cast<uint32_t>(tlasMaxInstances)) {
			// Full rebuild needed - release old resources first to avoid leaks
			destroyTLASResources();
			BuildTLAS();
			// Update descriptor set
			if (tlas != VK_NULL_HANDLE && descSet != VK_NULL_HANDLE) {
				VkWriteDescriptorSetAccelerationStructureKHR writeDescAS{};
				writeDescAS.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
				writeDescAS.accelerationStructureCount = 1;
				writeDescAS.pAccelerationStructures = &tlas;

				VkWriteDescriptorSet descSetWrite{};
				descSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descSetWrite.descriptorCount = 1;
				descSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
				descSetWrite.dstBinding = 0;
				descSetWrite.dstSet = descSet;
				descSetWrite.pNext = &writeDescAS;
				vkUpdateDescriptorSets(ctx->vkDevice(), 1, &descSetWrite, 0, nullptr);
			}
			return;
		}

		// Update instance buffer with new transforms
		tlasInstanceBuffer.copyFromCpu(ctx, instances.data(),
			sizeof(VkAccelerationStructureInstanceKHR) * primCount, 0);

		// Rebuild TLAS in-place (update mode)
		VkAccelerationStructureGeometryKHR asGeometry{};
		asGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		asGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		asGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		asGeometry.geometry.instances.data.deviceAddress = tlasInstanceBuffer.deviceAddress;
		asGeometry.geometry.instances.arrayOfPointers = VK_FALSE;

		VkAccelerationStructureBuildGeometryInfoKHR asBuildGeomInfo{};
		asBuildGeomInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		asBuildGeomInfo.geometryCount = 1;
		asBuildGeomInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
		asBuildGeomInfo.pGeometries = &asGeometry;
		asBuildGeomInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		asBuildGeomInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR
			| VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
		asBuildGeomInfo.srcAccelerationStructure = tlas;
		asBuildGeomInfo.dstAccelerationStructure = tlas;
		asBuildGeomInfo.scratchData.deviceAddress = tlasScratchBuffer.deviceAddress;

		VkCommandBufferManager cmdBufferManager(ctx);
		cmdBufferManager.allocate(1);
		cmdBufferManager.begin();

		VkAccelerationStructureBuildRangeInfoKHR buildrangeAS{};
		buildrangeAS.primitiveCount = primCount;
		std::vector<VkAccelerationStructureBuildRangeInfoKHR*> buildRange{ &buildrangeAS };
		vkCmdBuildAccelerationStructuresKHR(cmdBufferManager.get(), 1, &asBuildGeomInfo, buildRange.data());

		cmdBufferManager.end();
		cmdBufferManager.submitWithFence(tlasFence);
	}
};
