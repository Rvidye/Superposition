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
		descSetLayout->writeToDescSet(index, 0, tlas);
	}

	Scene::Scene(const AMC::VkContext* vkctx) {
		ctx = vkctx;
		tlas = VK_NULL_HANDLE;
		descSet = VK_NULL_HANDLE;
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
			for (const auto& mesh : rendermodel.model->meshes) {
				asDeviceAI.accelerationStructure = mesh->blas;

				VkAccelerationStructureInstanceKHR inst{};
				inst.mask = 0xff;
				inst.accelerationStructureReference = vkGetAccelerationStructureDeviceAddressKHR(ctx->vkDevice(), &asDeviceAI);
				glm::mat3x4 id = glm::make_mat3x4(&glm::transpose(rendermodel.matrix)[0][0]);
				std::copy_n(&id[0][0], 3 * 4, &inst.transform.matrix[0][0]);
				instances.push_back(inst);
			}
		}
		
		AMC::Buffer instanceBuffer = mem->createBuffer(sizeof(VkAccelerationStructureInstanceKHR) * instances.size(), AMC::MemoryFlags::kVkMemoryBit, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, true);
		instanceBuffer.copyFromCpu(ctx, instances, 0);

		VkAccelerationStructureGeometryKHR asGeometry{};
		asGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		asGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		asGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		asGeometry.geometry.instances.data.deviceAddress = instanceBuffer.deviceAddress;
		asGeometry.geometry.instances.arrayOfPointers = VK_FALSE;

		VkAccelerationStructureBuildGeometryInfoKHR asBuildGeomInfo{};
		asBuildGeomInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		asBuildGeomInfo.geometryCount = 1;
		asBuildGeomInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		asBuildGeomInfo.pGeometries = &asGeometry;
		asBuildGeomInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

		uint32_t primCount = static_cast<uint32_t>(instances.size());
		VkAccelerationStructureBuildSizesInfoKHR sizeInfo{};
		sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
		vkGetAccelerationStructureBuildSizesKHR(ctx->vkDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &asBuildGeomInfo, &primCount, &sizeInfo);

		AMC::Buffer scratch = mem->createBuffer(sizeInfo.buildScratchSize, AMC::MemoryFlags::kVkMemoryBit, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, true);
		asBuildGeomInfo.scratchData.deviceAddress = scratch.deviceAddress;

		AMC::Buffer asBuff = mem->createBuffer(sizeInfo.accelerationStructureSize, AMC::MemoryFlags::kVkMemoryBit, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR);

		VkAccelerationStructureCreateInfoKHR asCI{};
		asCI.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		asCI.size = sizeInfo.accelerationStructureSize;
		asCI.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		asCI.buffer = asBuff.vk;
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
		VkResult res = cmdBufferManager.submit();
		vkQueueWaitIdle(ctx->vkQueue());
	}
};