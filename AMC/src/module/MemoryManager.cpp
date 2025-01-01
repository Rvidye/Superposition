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
			//Prefer this over GL version
			if (vkmem != VK_NULL_HANDLE) {
				vkMapMemory(ctx->vkDevice(), vkmem, offset, size, 0, &mem);
				memcpy(mem, data, size);
			}
			else if (glIsBuffer(gl)) {
				glNamedBufferSubData(gl, offset, size, data);
			}
			else {
				LOG_ERROR(L"Invalid buffer used for copying");
			}
		}
	}

	void Image::transistionImageLayout(VkCommandBuffer cmdBuffer, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkImageLayout srcLayout, VkImageLayout dstLayout, VkImageSubresourceRange range) const {
		VkImageMemoryBarrier imageBarrier{};
		imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.srcAccessMask = srcAccess;
		imageBarrier.dstAccessMask = dstAccess;
		imageBarrier.oldLayout = srcLayout;
		imageBarrier.newLayout = dstLayout;
		imageBarrier.image = vk;
		imageBarrier.subresourceRange = range;
		vkCmdPipelineBarrier(cmdBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);
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
		const bool isVk = (memoryFlags & kVkMemoryBit) && ctx != nullptr;
		//TODO: Confirm if certain Buffers need to be updated per frame if that is the case: Provisions will have to be made on both Vulkan and OpenGL side.
		//Currently the Buffers are created to be un-modifyable by CPU for the sake of performance.

		if (isVk && ctx == nullptr) {
			LOG_ERROR(L"Cannot allocate a Vulkan Buffer without a context set.");
			return buffer;
		}

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
			bufferCI.pQueueFamilyIndices = &ctx->vkQueueFamilyIndex();
			bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			if (getAddress) {
				bufferUsage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
			}
			bufferCI.usage = bufferUsage;
			bufferCI.size = size;
			CHECK_VULKAN_ERROR(vkCreateBuffer(ctx->vkDevice(), &bufferCI, nullptr, &buffer.vk));

			VkMemoryRequirements memReq{};
			vkGetBufferMemoryRequirements(ctx->vkDevice(), buffer.vk, &memReq);

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
			memoryAI.memoryTypeIndex = getMemoryType(ctx->vkPhysicalDevice(), memReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			CHECK_VULKAN_ERROR(vkAllocateMemory(ctx->vkDevice(), &memoryAI, nullptr, &buffer.vkmem));

			CHECK_VULKAN_ERROR(vkBindBufferMemory(ctx->vkDevice(), buffer.vk, buffer.vkmem, 0));

			if (getAddress) {
				VkBufferDeviceAddressInfo bufferDeviceAI{};
				bufferDeviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
				bufferDeviceAI.buffer = buffer.vk;
				buffer.deviceAddress = vkGetBufferDeviceAddress(ctx->vkDevice(), &bufferDeviceAI);
			}

			if (isGl) {
				VkMemoryGetWin32HandleInfoKHR memGetWin32HI{};
				memGetWin32HI.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
				memGetWin32HI.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
				memGetWin32HI.memory = buffer.vkmem;
				CHECK_VULKAN_ERROR(vkGetMemoryWin32HandleKHR(ctx->vkDevice(), &memGetWin32HI, &buffer.memhandle));
				
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

	Image MemoryManager::createImage(VkExtent3D extent, VkFormat format, VkImageViewType type, uint32_t mipLevels, MemoryFlagBits memoryFlags, VkImageUsageFlags imageUsage) {
		Image image{};
		
		//Add as and when required
		const auto formatMap = [](const VkFormat format) -> const GLenum {
			switch (format) {
			case VK_FORMAT_R32G32B32A32_SFLOAT:
				return GL_RGBA32F;
			case VK_FORMAT_D32_SFLOAT:
				return GL_DEPTH_COMPONENT32F;
			default:
				LOG_ERROR(L"Unknown Format, Please add this format to Lambda");
				return GL_INVALID_ENUM;
			}
		};

		const auto vktypeMap = [](const VkImageViewType viewType) -> const VkImageType {
			switch(viewType) {
			case VK_IMAGE_VIEW_TYPE_1D:
				return VK_IMAGE_TYPE_1D;
			case VK_IMAGE_VIEW_TYPE_1D_ARRAY: case VK_IMAGE_VIEW_TYPE_2D:
				return VK_IMAGE_TYPE_2D;
			case VK_IMAGE_VIEW_TYPE_2D_ARRAY: case VK_IMAGE_VIEW_TYPE_3D: case VK_IMAGE_VIEW_TYPE_CUBE: case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
				return VK_IMAGE_TYPE_3D;
			default:
				LOG_ERROR(L"Shouldn't have reached here.");
				return VK_IMAGE_TYPE_MAX_ENUM;
			}
		};

		const auto aspectMap = [](const VkFormat format) -> const VkImageAspectFlags {
			switch (format) {
			case VK_FORMAT_D32_SFLOAT:
				return VK_IMAGE_ASPECT_DEPTH_BIT;
			default:
				return VK_IMAGE_ASPECT_COLOR_BIT;
			}
		};

		const bool isGl = memoryFlags & kGlMemoryBit;
		const bool isVk = (memoryFlags & kVkMemoryBit) && ctx != nullptr;
		if (isVk && ctx == nullptr) {
			LOG_ERROR(L"Cannot allocate a Vulkan Image without a context set.");
			return image;
		}

		if (isVk) {
			VkExternalMemoryImageCreateInfo externalMemoryImageCI{};
			externalMemoryImageCI.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
			externalMemoryImageCI.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;

			VkImageCreateInfo imageCI{};
			imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			if (isGl) {
				imageCI.pNext = &externalMemoryImageCI;
			}
			imageCI.queueFamilyIndexCount = 1;
			imageCI.pQueueFamilyIndices = &ctx->vkQueueFamilyIndex();
			imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageCI.usage = imageUsage;
			imageCI.arrayLayers = 1;
			imageCI.extent = extent;
			imageCI.format = format;
			imageCI.imageType = vktypeMap(type);
			imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageCI.mipLevels = mipLevels;
			imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
			imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
			CHECK_VULKAN_ERROR(vkCreateImage(ctx->vkDevice(), &imageCI, nullptr, &image.vk));

			VkMemoryRequirements memReq{};
			vkGetImageMemoryRequirements(ctx->vkDevice(), image.vk, &memReq);

			VkExportMemoryAllocateInfo exportMemoryAI{};
			exportMemoryAI.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
			exportMemoryAI.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;

			VkMemoryAllocateFlagsInfo memoryAIFlags{};
			memoryAIFlags.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
			memoryAIFlags.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

			VkMemoryAllocateInfo memoryAI{};
			memoryAI.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			if (isGl) {
				memoryAI.pNext = &exportMemoryAI;
			}
			memoryAI.allocationSize = memReq.size;
			memoryAI.memoryTypeIndex = getMemoryType(ctx->vkPhysicalDevice(), memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			CHECK_VULKAN_ERROR(vkAllocateMemory(ctx->vkDevice(), &memoryAI, nullptr, &image.vkmem));

			CHECK_VULKAN_ERROR(vkBindImageMemory(ctx->vkDevice(), image.vk, image.vkmem, 0));

			VkImageViewCreateInfo imageViewCI{};
			imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewCI.viewType = type;
			imageViewCI.components.r = VK_COMPONENT_SWIZZLE_R;
			imageViewCI.components.g = VK_COMPONENT_SWIZZLE_G;
			imageViewCI.components.b = VK_COMPONENT_SWIZZLE_B;
			imageViewCI.components.a = VK_COMPONENT_SWIZZLE_A;
			imageViewCI.format = format;
			imageViewCI.image = image.vk;
			imageViewCI.subresourceRange.aspectMask = aspectMap(format);
			imageViewCI.subresourceRange.layerCount = 1;
			imageViewCI.subresourceRange.levelCount = mipLevels;
			vkCreateImageView(ctx->vkDevice(), &imageViewCI, nullptr, &image.view);

			if (isGl) {
				VkMemoryGetWin32HandleInfoKHR memGetWin32HI{};
				memGetWin32HI.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
				memGetWin32HI.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
				memGetWin32HI.memory = image.vkmem;
				CHECK_VULKAN_ERROR(vkGetMemoryWin32HandleKHR(ctx->vkDevice(), &memGetWin32HI, &image.memhandle));

				glCreateMemoryObjectsEXT(1, &image.glmem);
				glImportMemoryWin32HandleEXT(image.glmem, memReq.size, GL_HANDLE_TYPE_OPAQUE_WIN32_EXT, image.memhandle);

				glCreateTextures(GL_TEXTURE_2D, 1, &image.gl);
				switch(type) {
				case VK_IMAGE_VIEW_TYPE_1D:
					glTextureStorageMem1DEXT(image.gl, mipLevels, formatMap(format), extent.width, image.glmem, 0);
					break;
				case VK_IMAGE_VIEW_TYPE_2D:
					glTextureStorageMem2DEXT(image.gl, mipLevels, formatMap(format), extent.width, extent.height, image.glmem, 0);
					break;
				case VK_IMAGE_VIEW_TYPE_3D:
					glTextureStorageMem3DEXT(image.gl, mipLevels, formatMap(format), extent.width, extent.height, extent.depth, image.glmem, 0);
					break;
				}
			}
		}
		else if (isGl) {
			glCreateTextures(GL_TEXTURE_2D, 1, &image.gl);
			switch (type) {
			case VK_IMAGE_VIEW_TYPE_1D:
				glTextureStorage1D(image.gl, mipLevels, formatMap(format), extent.width);
				break;
			case VK_IMAGE_VIEW_TYPE_2D:
				glTextureStorage2D(image.gl, mipLevels, formatMap(format), extent.width, extent.height);
				break;
			case VK_IMAGE_VIEW_TYPE_3D:
				glTextureStorage3D(image.gl, mipLevels, formatMap(format), extent.width, extent.height, extent.depth);
				break;
			}
		}
		else {
			LOG_ERROR(L"Invalid MemoryType, Make sure to specify at one of the following: kGlMemory, kVkMemory");
		}

		return image;
	}
};