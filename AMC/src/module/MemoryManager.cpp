#include "MemoryManager.h"
#include "Log.h"


static VkResult result;

#ifdef _MYDEBUG
#define CHECK_VULKAN_ERROR(X) if((result = X) != VK_SUCCESS) {LOG_ERROR(L"Vulkan: " #X "at %d failed with %d", __LINE__, result); return {};}
#else
#define CHECK_VULKAN_ERROR(X) X;
#endif // _MYDEBUG

namespace AMC {
	void Buffer::copyFromCpu(const VkContext* ctx, const void* data, size_t size, size_t offset, bool useStaging) const {
		if (!useStaging) {
			void* mem;
			vkMapMemory(ctx->vkDevice(), vkmem, offset, size, 0, &mem);
			memcpy(mem, data, size);
		}
	}

	static uint32_t getMemoryType(VkPhysicalDevice dev, uint32_t typeBits, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memoryProperties;
		vkGetPhysicalDeviceMemoryProperties(dev, &memoryProperties);
		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
			if ((typeBits & 1) == 1 && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
			typeBits >>= 1;
		}
		return 0xffff;
	}

	Buffer MemoryManager::createBuffer(uint64_t size, MemoryFlagBits memoryFlags, VkBufferUsageFlags bufferUsage, bool getAddress) {
		Buffer buffer{};
		
		const bool isGl = memoryFlags & kGlMemoryBit;
		const bool isVk = memoryFlags & kVkMemoryBit;
		//TODO: Confirm if certain Buffers need to be updated per frame if that is the case: Provisions will have to be made on both Vulkan and OpenGL side.
		//Currently the Buffers are created to be un-modifyable by CPU for the sake of performance.

		if (isVk) {
			VkExternalMemoryBufferCreateInfo externalMemoryBufferCI{};
			externalMemoryBufferCI.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO;
			externalMemoryBufferCI.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;

			VkBufferCreateInfo bufferCI{};
			bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			if (isGl) {
				bufferCI.pNext = &externalMemoryBufferCI;
			}
			bufferCI.queueFamilyIndexCount = 1;
			bufferCI.pQueueFamilyIndices = &ctx.vkQueueFamilyIndex();
			bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			if (getAddress) {
				bufferUsage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
			}
			bufferCI.usage = bufferUsage;
			bufferCI.size = size;
			CHECK_VULKAN_ERROR(vkCreateBuffer(ctx.vkDevice(), &bufferCI, nullptr, &buffer.vk));

			VkMemoryRequirements memReq{};
			vkGetBufferMemoryRequirements(ctx.vkDevice(), buffer.vk, &memReq);

			VkExportMemoryAllocateInfo exportMemoryAI{};
			exportMemoryAI.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
			exportMemoryAI.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;

			VkMemoryAllocateFlagsInfo memoryAIFlags{};
			memoryAIFlags.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
			memoryAIFlags.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

			VkMemoryAllocateInfo memoryAI{};
			memoryAI.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			const void **pNext = &memoryAI.pNext;
			if (isGl) {
				*pNext = &exportMemoryAI;
				pNext = &exportMemoryAI.pNext;
			}
			if (getAddress) {
				*pNext = &memoryAIFlags;
				pNext = &memoryAIFlags.pNext;
			}
			memoryAI.allocationSize = memReq.size;
			memoryAI.memoryTypeIndex = getMemoryType(ctx.vkPhysicalDevice(), memReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			CHECK_VULKAN_ERROR(vkAllocateMemory(ctx.vkDevice(), &memoryAI, nullptr, &buffer.vkmem));

			CHECK_VULKAN_ERROR(vkBindBufferMemory(ctx.vkDevice(), buffer.vk, buffer.vkmem, 0));

			if (getAddress) {
				VkBufferDeviceAddressInfo bufferDeviceAI{};
				bufferDeviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
				bufferDeviceAI.buffer = buffer.vk;
				buffer.deviceAddress = vkGetBufferDeviceAddress(ctx.vkDevice(), &bufferDeviceAI);
			}

			if (isGl) {
				VkMemoryGetWin32HandleInfoKHR memGetWin32HI{};
				memGetWin32HI.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
				memGetWin32HI.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
				memGetWin32HI.memory = buffer.vkmem;
				CHECK_VULKAN_ERROR(vkGetMemoryWin32HandleKHR(ctx.vkDevice(), &memGetWin32HI, &buffer.memhandle));
				
				glCreateMemoryObjectsEXT(1, &buffer.glmem);
				glImportMemoryWin32HandleEXT(buffer.glmem, memReq.size, GL_HANDLE_TYPE_OPAQUE_WIN32_EXT, buffer.memhandle);

				glCreateBuffers(1, &buffer.gl);
				glNamedBufferStorageMemEXT(buffer.gl, size, buffer.glmem, 0);
			}
		}
		else if(isGl) {
			glCreateBuffers(1, &buffer.gl);
			glNamedBufferStorage(buffer.gl, size, nullptr, 0);
		}
		else {
			LOG_ERROR(L"Invalid MemoryType, Make sure to specify at one of the following: kGlMemory, kVkMemory");
		}

		return buffer;
	}
};