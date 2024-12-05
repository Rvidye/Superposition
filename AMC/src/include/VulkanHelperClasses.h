#pragma once

#include<vector>
#include<string>
#include<unordered_map>
#include<variant>

#include<vulkan/vulkan.h>

namespace AMC {
	class VkContext
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
			VkContext* build();
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
		const VkPhysicalDevice& vkPhysicalDevice() const { return physicalDevice; }
		const VkDevice& vkDevice() const { return device; }
		const VkQueue& vkQueue() const { return queue; }
		const VkCommandPool& vkCommandPool() const { return commandPool; }

		~VkContext();
	private:
		VkContext();
		VkInstance instance;
		VkPhysicalDevice physicalDevice;
		VkDevice device;
		VkQueue queue;
		VkCommandPool commandPool;
	};

	class VkDescSetManager {
	public:
		class Builder {
		public:
			Builder(const VkContext* context);
			Builder& addToDescSet(std::string key, uint32_t binding, VkDescriptorType descType, VkShaderStageFlags stageFlags, std::vector<VkBuffer> bufferVector);
			VkDescSetManager* build();
		private:
			struct DescriptorSetData {
				uint32_t binding;
				VkDescriptorType type;
				VkShaderStageFlags shaderStages;
				std::vector<std::variant<VkBuffer, VkImageView>> resourceVec;

				DescriptorSetData(uint32_t binding, VkDescriptorType type, VkShaderStageFlags shaderStages, std::vector<VkBuffer> bufferVector)
				: binding(binding), type(type), shaderStages(shaderStages), resourceVec(bufferVector.begin(), bufferVector.end())
				{
				}
			};
			const VkContext* ctx;
			std::unordered_map<std::string, std::vector<DescriptorSetData>> descSetData;
		};

		VkDescriptorSetLayout vkDescriptorSetLayout(std::string key) const { return descriptorSet.at(key).first; }
	private:
		VkDescSetManager() : descriptorSet({}), pool(VK_NULL_HANDLE) {};

		std::unordered_map<std::string, std::pair<VkDescriptorSetLayout, std::vector<VkDescriptorSet>>> descriptorSet;
		VkDescriptorPool pool;
	};
}

