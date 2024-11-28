#pragma once

#include<vector>

#include<vulkan/vulkan.h>

namespace AMC {
	class VulkanContext
	{
	public:
		VulkanContext();

		VulkanContext* setAPIVersion(uint32_t apiVersion);
		VulkanContext* addInstanceExtension(const char* extension);
		VulkanContext* addInstanceLayer(const char* layer);
		VulkanContext* setSelectedDevice(uint32_t deviceIndex);
		VulkanContext* setRequiredQueueFlags(VkQueueFlags queueFlags);
		VulkanContext* addDeviceExtension(const char* extension);
		VulkanContext* setDeviceFeatureStruct(VkPhysicalDeviceFeatures deviceFeature);
		VulkanContext* addToDeviceFeatureChain(void* deviceFeature);
		void build();
		
		~VulkanContext();
	private:
		VkInstance instance;
		VkPhysicalDevice physicalDevice;
		VkDevice device;
		VkQueue queue;
		VkCommandPool commandPool;
		VkDescriptorPool descriptorPool;

		uint32_t apiVersion;
		std::vector<const char*> instanceExtensions;
		std::vector<const char*> instanceLayers;

		uint32_t selectedDevice;

		VkQueueFlags requestedQueueFlags;
		std::vector<const char*> deviceExtensions;
		VkPhysicalDeviceFeatures2 deviceFeatures;

		std::vector<VkDescriptorPoolSize> descPoolSizes;
		uint32_t descSetCount;
	};
}

