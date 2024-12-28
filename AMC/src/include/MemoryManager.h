#pragma once

#include<GL/glew.h>
#define VK_USE_PLATFORM_WIN32_KHR
#include<volk.h>

#include "VulkanHelperClasses.h"

//TODO: Add VMA or write a better memory allocator to clump up device memory instead of allocating memory for every call
namespace AMC {
	enum MemoryFlags {
		kGlMemoryBit = 0b01,
		kVkMemoryBit = 0b10
	};
	using MemoryFlagBits = uint32_t;

	struct Buffer {
		GLuint gl;
		VkBuffer vk;
		VkDeviceMemory vkmem;
		GLuint glmem;
		HANDLE memhandle;
		VkDeviceAddress deviceAddress;
		//Just for VK for now
		void copyFromCpu(const VkContext* ctx, const void* data, size_t size, size_t offset, bool useStaging = false) const;
		template<typename T>
		void copyFromCpu(const VkContext* ctx, const std::vector<T>& dataVec, size_t offset, bool useStaging = false) const {
			copyFromCpu(ctx, dataVec.data(), dataVec.size() * sizeof(T), offset, useStaging);
		}
	};

	struct Image {
		GLuint gl;
		VkImage vk;
		VkImageView view;
		VkDeviceMemory vkmem;
		GLuint glmem;
		HANDLE memhandle;
		void transistionImageLayout(VkCommandBuffer cmdBuffer, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkImageLayout srcLayout, VkImageLayout dstLayout, VkImageSubresourceRange range) const;
	};

	class MemoryManager {
	public:
		MemoryManager(const VkContext* ctx) : ctx(ctx) {}
		//TODO: Make MemoryFlags a Template argument to improve perf, Low Priority
		Buffer createBuffer(uint64_t size, MemoryFlagBits memoryFlags, VkBufferUsageFlags bufferUsage = 0, bool getAddress = false);
		Image createImage(VkExtent3D extent, VkFormat format, VkImageViewType type, uint32_t mipLevels, MemoryFlagBits memoryFlags, VkImageUsageFlags imageUsage = 0);
	private:
		const VkContext* ctx;
	};
};