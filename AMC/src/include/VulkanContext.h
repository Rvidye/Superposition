#pragma once

#include<vector>

#include<vulkan/vulkan.h>

namespace AMC {
	class VulkanContext
	{
	public:
		class Builder {
		public:
			Builder();
			Builder& setAPIVersion(uint32_t apiVersion);
			Builder& addInstanceExtension(const char* extension);
			Builder& addInstanceLayer(const char* layer);
			Builder& setSelectedDevice(uint32_t deviceIndex);
			Builder& setRequiredQueueFlags(VkQueueFlags queueFlags);
			Builder& addDeviceExtension(const char* extension);
			Builder& setDeviceFeatureStruct(VkPhysicalDeviceFeatures deviceFeature);
			Builder& addToDeviceFeatureChain(void* deviceFeature);
			VulkanContext* build();
			~Builder();
		private:
			uint32_t apiVersion;
			std::vector<const char*> instanceExtensions;
			std::vector<const char*> instanceLayers;

			uint32_t selectedDevice;

			VkQueueFlags requestedQueueFlags;
			std::vector<const char*> deviceExtensions;
			VkPhysicalDeviceFeatures2 deviceFeatures;
		};

		const VkInstance& vkInstance() const { return instance; }
		const VkPhysicalDevice& vkphysicalDevice() const { return physicalDevice; }
		const VkDevice& vkdevice() const { return device; }
		const VkQueue& vkqueue() const { return queue; }
		const VkCommandPool& vkcommandPool() const { return commandPool; }

		~VulkanContext();
	private:
		VulkanContext();
		VkInstance instance;
		VkPhysicalDevice physicalDevice;
		VkDevice device;
		VkQueue queue;
		VkCommandPool commandPool;
	};
}

