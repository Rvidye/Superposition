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
	};

	class MemoryManager {
	public:
		MemoryManager(const VkContext& ctx) : ctx(ctx) {}
		//TODO: Make MemoryFlags a Template argument to improve perf, Low Priority
		Buffer createBuffer(uint64_t size, MemoryFlagBits memoryFlags, VkBufferUsageFlags bufferUsage);
	private:
		const VkContext& ctx;
	};
};