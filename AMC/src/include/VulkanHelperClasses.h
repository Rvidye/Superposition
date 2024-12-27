#pragma once

#include<vector>
#include<string>
#include<unordered_map>
#include<variant>

#define VK_USE_PLATFORM_WIN32_KHR
#include<volk.h>

#define RT_ENABLE

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
		const uint32_t& vkQueueFamilyIndex() const { return queueFamilyIndex; }
		const VkCommandPool& vkCommandPool() const { return commandPool; }

		~VkContext();
	private:
		VkContext();
		VkInstance instance;
		VkPhysicalDevice physicalDevice;
		VkDevice device;
		VkQueue queue;
		uint32_t queueFamilyIndex;
		VkCommandPool commandPool;
		VkDebugUtilsMessengerEXT callback;
	};

	class VkDescSetLayoutManager {
	public:
		VkDescSetLayoutManager(const AMC::VkContext* vkctx, uint32_t count = 1) : ctx(vkctx), descSetLayout(VK_NULL_HANDLE), descSetCount(count) {}
		void addBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags shaderStages);
		void generateDescSetLayout();
		static void generateDescSetPool(const VkContext* ctx);
		void generateDescSet();
		void writeToDescSet(uint32_t index, uint32_t binding, std::variant<VkAccelerationStructureKHR, VkDescriptorImageInfo> obj);
		VkDescriptorSetLayout vkDescSetLayout() const { return descSetLayout; };
		VkDescriptorSet vkDescSet(uint32_t index) { return descSet[index]; }
	private:
		const VkContext* ctx;
		std::vector<VkDescriptorSetLayoutBinding> bindings;
		VkDescriptorSetLayout descSetLayout;
		uint32_t descSetCount;
		std::vector<VkDescriptorSet> descSet;
		//Currently It is made for a single pool for all contexts. 
		//TODO: convert this to be a pool for each context
		static std::unordered_map<VkDescriptorType, uint32_t> poolsizeMap;
		static VkDescriptorPool descPool;
		static uint32_t maxSetCount;
	};

	class VkCommandBufferManager {
	public:
		static constexpr uint32_t kFreeAll = UINT32_MAX;
		VkCommandBufferManager(const VkContext* ctx);
		void allocate(uint32_t count = 1);
		VkCommandBuffer get(uint32_t index = 0) const;
		VkResult submit(uint32_t index = 0);
		VkResult begin(uint32_t index = 0);
		VkResult end(uint32_t index = 0);
		void free(uint32_t count = kFreeAll);
	private:
		const VkContext* vkctx;
		std::vector<VkCommandBuffer> commandBufferList{};
	};

	VkShaderModule loadShaderModule(const VkContext* ctx, std::string fileName);
}

