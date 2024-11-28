#include "VulkanContext.h"
#include "Log.h"

static VkResult result;

#ifdef _MYDEBUG
#define CHECK_VULKAN_ERROR(X) if((result = X) != VK_SUCCESS) {LOG_ERROR(L"Vulkan: " #X "at %d failed with %d", __LINE__, result); return;}
#else
#define CHECK_VULKAN_ERROR(X) X;
#endif // _MYDEBUG


namespace AMC {
	VulkanContext::VulkanContext() {
		instance = VK_NULL_HANDLE;
		physicalDevice = VK_NULL_HANDLE;
		device = VK_NULL_HANDLE;
		commandPool = VK_NULL_HANDLE;
		descriptorPool = VK_NULL_HANDLE;

		instanceLayers = {};
		instanceExtensions = {};
		apiVersion = VK_VERSION_1_0;

		selectedDevice = 0;

		requestedQueueFlags = VK_QUEUE_FLAG_BITS_MAX_ENUM;
		deviceExtensions = {};
		deviceFeatures = {};
		deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

		descPoolSizes = {};
		descSetCount = 1;
	}

	VulkanContext* VulkanContext::setAPIVersion(uint32_t apiv) {
		apiVersion = apiv;
		return this;
	}

	VulkanContext* VulkanContext::addInstanceExtension(const char* extension) {
		instanceExtensions.push_back(extension);
		return this;
	}

	VulkanContext* VulkanContext::addInstanceLayer(const char* layer) {
		instanceLayers.push_back(layer);
		return this;
	}

	VulkanContext* VulkanContext::setSelectedDevice(uint32_t deviceIndex) {
		selectedDevice = deviceIndex;
		return this;
	}

	VulkanContext* VulkanContext::setRequiredQueueFlags(VkQueueFlags queueFlags) {
		requestedQueueFlags = queueFlags;
		return this;
	}

	VulkanContext* VulkanContext::addDeviceExtension(const char* extension) {
		deviceExtensions.push_back(extension);
		return this;
	}

	VulkanContext* VulkanContext::setDeviceFeatureStruct(VkPhysicalDeviceFeatures deviceFeature) {
		deviceFeatures.features = deviceFeature;
		return this;
	}

	VulkanContext* VulkanContext::addToDeviceFeatureChain(void* deviceFeature) {
		struct InfoStruct {
			VkStructureType sType;
			void* pNext;
		};
		InfoStruct* featureStruct = reinterpret_cast<InfoStruct*>(deviceFeature);
		featureStruct->pNext = deviceFeatures.pNext;
		deviceFeatures.pNext = featureStruct;
		return this;
	}

	void VulkanContext::build() {
		//Instance Creation
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.apiVersion = apiVersion;
		appInfo.engineVersion = 1;
		appInfo.applicationVersion = 1;
		appInfo.pEngineName = "AMC";
		appInfo.pApplicationName = "Raster";

#ifdef _MYDEBUG
		instanceLayers.push_back("VK_LAYER_KHRONOS_validation");
		LOG_WARNING(L"Vulkan: Enabling Validation Layers");
#endif // _MYDEBUG

		VkInstanceCreateInfo instanceCI{};
		instanceCI.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCI.pApplicationInfo = &appInfo;
		instanceCI.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
		instanceCI.ppEnabledExtensionNames = instanceExtensions.data();
		instanceCI.enabledLayerCount = static_cast<uint32_t>(instanceLayers.size());
		instanceCI.ppEnabledLayerNames = instanceLayers.data();
		CHECK_VULKAN_ERROR(vkCreateInstance(&instanceCI, nullptr, &instance));

		uint32_t devCount;
		CHECK_VULKAN_ERROR(vkEnumeratePhysicalDevices(instance, &devCount, nullptr));
		std::vector<VkPhysicalDevice> devList(devCount);
		CHECK_VULKAN_ERROR(vkEnumeratePhysicalDevices(instance, &devCount, devList.data()));
		selectedDevice = std::min(selectedDevice, devCount - 1);
		physicalDevice = devList[selectedDevice];

		VkPhysicalDeviceProperties props{};
		vkGetPhysicalDeviceProperties(physicalDevice, &props);

		LOG_WARNING(L"Vulkan: Using %s device. Ensure OpenGL is running on the same device.", CString(props.deviceName));
	
		VkDeviceQueueCreateInfo queueCreateInfo{};
		const float queuePriorities = 1.0f;

		uint32_t queueFamilyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilyProps(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProps.data());

		uint32_t queueFamilyIndex = 0;
		for (uint32_t i = 0; i < queueFamilyCount; i++) {
			if (queueFamilyProps[i].queueFlags & requestedQueueFlags) {
				queueFamilyIndex = i;
				break;
			}
		}
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriorities;

		VkDeviceCreateInfo deviceCI{};
		deviceCI.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCI.pNext = &deviceFeatures;
		deviceCI.queueCreateInfoCount = 1;
		deviceCI.pQueueCreateInfos = &queueCreateInfo;
		deviceCI.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		deviceCI.ppEnabledExtensionNames = deviceExtensions.data();
		CHECK_VULKAN_ERROR(vkCreateDevice(physicalDevice, &deviceCI, nullptr, &device));
	
		vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);

		VkCommandPoolCreateInfo commandPoolInfo{};
		commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		commandPoolInfo.queueFamilyIndex = queueFamilyIndex;
		CHECK_VULKAN_ERROR(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &commandPool));
		
		if (descPoolSizes.size()) {
			descSetCount = std::max(descSetCount, 1u);
			VkDescriptorPoolCreateInfo descPoolCI{};
			descPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			descPoolCI.poolSizeCount = static_cast<uint32_t>(descPoolSizes.size());
			descPoolCI.pPoolSizes = descPoolSizes.data();
			descPoolCI.maxSets = descSetCount;
			CHECK_VULKAN_ERROR(vkCreateDescriptorPool(device, &descPoolCI, nullptr, &descriptorPool));
		}

		LOG_INFO(L"Vulkan: Context Successfully Initialized");
	}

	VulkanContext::~VulkanContext() {
		if (descriptorPool != VK_NULL_HANDLE) {
			vkDestroyDescriptorPool(device, descriptorPool, nullptr);
		}
		if (commandPool != VK_NULL_HANDLE) {
			vkDestroyCommandPool(device, commandPool, nullptr);
		}
		if (device != VK_NULL_HANDLE) {
			vkDestroyDevice(device, nullptr);
		}
		if (instance != VK_NULL_HANDLE) {
			vkDestroyInstance(instance, nullptr);
		}
	}
}