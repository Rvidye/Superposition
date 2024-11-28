#include "VulkanContext.h"
#include "Log.h"

static VkResult result;

#ifdef _MYDEBUG
#define CHECK_VULKAN_ERROR(X) if((result = X) != VK_SUCCESS) {LOG_ERROR(L"Vulkan: " #X "at %d failed with %d", __LINE__, result); return nullptr;}
#else
#define CHECK_VULKAN_ERROR(X) X;
#endif // _MYDEBUG


namespace AMC {
	VulkanContext::Builder::Builder() {
		instanceLayers = {};
		instanceExtensions = {};
		apiVersion = VK_VERSION_1_0;

		selectedDevice = 0;

		requestedQueueFlags = VK_QUEUE_FLAG_BITS_MAX_ENUM;
		deviceExtensions = {};
		deviceFeatures = {};
		deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	}

	VulkanContext::Builder& VulkanContext::Builder::setAPIVersion(uint32_t apiv) {
		apiVersion = apiv;
		return *this;
	}

	VulkanContext::Builder& VulkanContext::Builder::addInstanceExtension(const char* extension) {
		instanceExtensions.push_back(extension);
		return *this;
	}

	VulkanContext::Builder& VulkanContext::Builder::addInstanceLayer(const char* layer) {
		instanceLayers.push_back(layer);
		return *this;
	}

	VulkanContext::Builder& VulkanContext::Builder::setSelectedDevice(uint32_t deviceIndex) {
		selectedDevice = deviceIndex;
		return *this;
	}

	VulkanContext::Builder& VulkanContext::Builder::setRequiredQueueFlags(VkQueueFlags queueFlags) {
		requestedQueueFlags = queueFlags;
		return *this;
	}

	VulkanContext::Builder& VulkanContext::Builder::addDeviceExtension(const char* extension) {
		deviceExtensions.push_back(extension);
		return *this;
	}

	VulkanContext::Builder& VulkanContext::Builder::setDeviceFeatureStruct(VkPhysicalDeviceFeatures deviceFeature) {
		deviceFeatures.features = deviceFeature;
		return *this;
	}

	VulkanContext::Builder& VulkanContext::Builder::addToDeviceFeatureChain(void* deviceFeature) {
		struct InfoStruct {
			VkStructureType sType;
			void* pNext;
		};
		InfoStruct* featureStruct = reinterpret_cast<InfoStruct*>(deviceFeature);
		featureStruct->pNext = deviceFeatures.pNext;
		deviceFeatures.pNext = featureStruct;
		return *this;
	}

	VulkanContext* VulkanContext::Builder::build() {
		VulkanContext* ctx = new VulkanContext();

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
		CHECK_VULKAN_ERROR(vkCreateInstance(&instanceCI, nullptr, &ctx->instance));

		uint32_t devCount;
		CHECK_VULKAN_ERROR(vkEnumeratePhysicalDevices(ctx->instance, &devCount, nullptr));
		std::vector<VkPhysicalDevice> devList(devCount);
		CHECK_VULKAN_ERROR(vkEnumeratePhysicalDevices(ctx->instance, &devCount, devList.data()));
		selectedDevice = std::min(selectedDevice, devCount - 1);
		ctx->physicalDevice = devList[selectedDevice];

		VkPhysicalDeviceProperties props{};
		vkGetPhysicalDeviceProperties(ctx->physicalDevice, &props);

		LOG_WARNING(L"Vulkan: Using %s device. Ensure OpenGL is running on the same device.", CString(props.deviceName));
	
		VkDeviceQueueCreateInfo queueCreateInfo{};
		const float queuePriorities = 1.0f;

		uint32_t queueFamilyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(ctx->physicalDevice, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilyProps(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(ctx->physicalDevice, &queueFamilyCount, queueFamilyProps.data());

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
		CHECK_VULKAN_ERROR(vkCreateDevice(ctx->physicalDevice, &deviceCI, nullptr, &ctx->device));
	
		vkGetDeviceQueue(ctx->device, queueFamilyIndex, 0, &ctx->queue);

		VkCommandPoolCreateInfo commandPoolInfo{};
		commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		commandPoolInfo.queueFamilyIndex = queueFamilyIndex;
		CHECK_VULKAN_ERROR(vkCreateCommandPool(ctx->device, &commandPoolInfo, nullptr, &ctx->commandPool));
		
		LOG_INFO(L"Vulkan: Context Successfully Initialized");
		return ctx;
	}

	VulkanContext::Builder::~Builder() {

	}

	VulkanContext::VulkanContext() {
		instance = VK_NULL_HANDLE;
		physicalDevice = VK_NULL_HANDLE;
		device = VK_NULL_HANDLE;
		commandPool = VK_NULL_HANDLE;
	}

	VulkanContext::~VulkanContext() {
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