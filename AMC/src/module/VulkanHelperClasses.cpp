#include "VulkanHelperClasses.h"
#include "Log.h"
#include<fstream>

static VkResult result;

#ifdef _MYDEBUG
#define CHECK_VULKAN_ERROR(X) if((result = X) != VK_SUCCESS) {LOG_ERROR(L"Vulkan: " #X "at %d failed with %d", __LINE__, result); return nullptr;}
#else
#define CHECK_VULKAN_ERROR(X) X;
#endif // _MYDEBUG


namespace AMC {
	VkContext::Builder::Builder() {
		instanceLayers = {};
		instanceExtensions = {};
		apiVersion = VK_VERSION_1_0;

		selectedDevice = 0;

		requestedQueueFlags = VK_QUEUE_FLAG_BITS_MAX_ENUM;
		deviceExtensions = {};
		deviceFeatures = {};
		deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	}

	VkContext::Builder& VkContext::Builder::setAPIVersion(uint32_t apiv) {
		apiVersion = apiv;
		return *this;
	}

	VkContext::Builder& VkContext::Builder::addInstanceExtension(const char* extension) {
		instanceExtensions.push_back(extension);
		return *this;
	}

	VkContext::Builder& VkContext::Builder::addInstanceLayer(const char* layer) {
		instanceLayers.push_back(layer);
		return *this;
	}

	VkContext::Builder& VkContext::Builder::setSelectedDevice(uint32_t deviceIndex) {
		selectedDevice = deviceIndex;
		return *this;
	}

	VkContext::Builder& VkContext::Builder::setRequiredQueueFlags(VkQueueFlags queueFlags) {
		requestedQueueFlags = queueFlags;
		return *this;
	}

	VkContext::Builder& VkContext::Builder::addDeviceExtension(const char* extension) {
		deviceExtensions.push_back(extension);
		return *this;
	}

	VkContext::Builder& VkContext::Builder::setDeviceFeatureStruct(VkPhysicalDeviceFeatures deviceFeature) {
		deviceFeatures.features = deviceFeature;
		return *this;
	}

	VkContext::Builder& VkContext::Builder::addToDeviceFeatureChain(void* deviceFeature) {
		struct InfoStruct {
			VkStructureType sType;
			void* pNext;
		};
		InfoStruct* featureStruct = reinterpret_cast<InfoStruct*>(deviceFeature);
		featureStruct->pNext = deviceFeatures.pNext;
		deviceFeatures.pNext = featureStruct;
		return *this;
	}

	static VkBool32 VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
		switch (messageSeverity) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			LOG_ERROR(L"Vulkan Error: %s", CString(pCallbackData->pMessage));
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			LOG_WARNING(L"Vulkan Warning: %s", CString(pCallbackData->pMessage));
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			LOG_INFO(L"Vulkan Info: %s", CString(pCallbackData->pMessage));
			break;
		}

		return VK_FALSE;
	}

	VkContext* VkContext::Builder::build() {
		VkContext* ctx = new VkContext();

		volkInitialize();

		//Instance Creation
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.apiVersion = apiVersion;
		appInfo.engineVersion = 1;
		appInfo.applicationVersion = 1;
		appInfo.pEngineName = "AMC";
		appInfo.pApplicationName = "Raster";

#ifdef _MYDEBUG
		instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
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

		volkLoadInstance(ctx->instance);

#ifdef _MYDEBUG
		VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCI{};
		debugUtilsMessengerCI.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugUtilsMessengerCI.pfnUserCallback = VulkanDebugCallback;
		debugUtilsMessengerCI.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
		debugUtilsMessengerCI.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

		vkCreateDebugUtilsMessengerEXT(ctx->instance, &debugUtilsMessengerCI, nullptr, &ctx->callback);
#endif // _MYDEBUG

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

		for (uint32_t i = 0; i < queueFamilyCount; i++) {
			if (queueFamilyProps[i].queueFlags & requestedQueueFlags) {
				ctx->queueFamilyIndex = i;
				break;
			}
		}
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = ctx->queueFamilyIndex;
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

		volkLoadDevice(ctx->device);

		vkGetDeviceQueue(ctx->device, ctx->queueFamilyIndex, 0, &ctx->queue);

		VkCommandPoolCreateInfo commandPoolInfo{};
		commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		commandPoolInfo.queueFamilyIndex = ctx->vkQueueFamilyIndex();
		CHECK_VULKAN_ERROR(vkCreateCommandPool(ctx->device, &commandPoolInfo, nullptr, &ctx->commandPool));

		LOG_INFO(L"Vulkan: Context Successfully Initialized");
		return ctx;
	}

	VkContext::Builder::~Builder() {

	}

	VkContext::VkContext() {
		instance = VK_NULL_HANDLE;
		physicalDevice = VK_NULL_HANDLE;
		device = VK_NULL_HANDLE;
		queueFamilyIndex = 0xffff;
		queue = VK_NULL_HANDLE;
		commandPool = VK_NULL_HANDLE;
		callback = VK_NULL_HANDLE;
	}

	VkContext::~VkContext() {
		if (commandPool != VK_NULL_HANDLE) {
			vkDestroyCommandPool(device, commandPool, nullptr);
		}
		if (device != VK_NULL_HANDLE) {
			vkDestroyDevice(device, nullptr);
		}
		if (callback != VK_NULL_HANDLE) {
			vkDestroyDebugUtilsMessengerEXT(instance, callback, nullptr);
		}
		if (instance != VK_NULL_HANDLE) {
			vkDestroyInstance(instance, nullptr);
		}
	}

	std::unordered_map<VkDescriptorType, uint32_t> VkDescSetLayoutManager::poolsizeMap{};
	VkDescriptorPool VkDescSetLayoutManager::descPool = VK_NULL_HANDLE;
	uint32_t VkDescSetLayoutManager::maxSetCount = 0;

	void VkDescSetLayoutManager::addBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags shaderStages) {
		for (auto& bind : bindings)
		{
			if(bind.binding == binding) {
				LOG_ERROR(L"Binding %d already present in Descriptor Set.", binding);
				return;
			}
		}

		VkDescriptorSetLayoutBinding bind{};
		bind.binding = binding;
		bind.descriptorType = type;
		bind.stageFlags = shaderStages;
		bind.descriptorCount = 1;
		bindings.push_back(bind);

		poolsizeMap[type] += descSetCount;

		maxSetCount += descSetCount;
	}

	void VkDescSetLayoutManager::generateDescSetLayout() {
		VkDescriptorSetLayoutCreateInfo descSetLayoutCI{};
		descSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descSetLayoutCI.bindingCount = static_cast<uint32_t>(bindings.size());
		descSetLayoutCI.pBindings = bindings.data();
		vkCreateDescriptorSetLayout(ctx->vkDevice(), &descSetLayoutCI, nullptr, &descSetLayout);
	}
	
	void VkDescSetLayoutManager::generateDescSetPool(const VkContext* ctx) {
		std::vector<VkDescriptorPoolSize> poolSizes{};
		for (auto& poolEntry : poolsizeMap) {
			poolSizes.push_back({ poolEntry.first, static_cast<uint32_t>(poolEntry.second) });
		}
		
		VkDescriptorPoolCreateInfo descPoolCI{};
		descPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descPoolCI.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		descPoolCI.pPoolSizes = poolSizes.data();
		descPoolCI.maxSets = maxSetCount;
		vkCreateDescriptorPool(ctx->vkDevice(), &descPoolCI, nullptr, &descPool);
	}

	void VkDescSetLayoutManager::generateDescSet() {
		std::vector<VkDescriptorSetLayout> layouts(descSetCount, descSetLayout);

		VkDescriptorSetAllocateInfo descAI{};
		descAI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descAI.descriptorPool = descPool;
		descAI.descriptorSetCount = descSetCount;
		descAI.pSetLayouts = layouts.data();

		descSet.resize(descSetCount);
		vkAllocateDescriptorSets(ctx->vkDevice(), &descAI, descSet.data());
	}

	void VkDescSetLayoutManager::writeToDescSet(uint32_t index, uint32_t binding, std::variant<VkAccelerationStructureKHR, VkDescriptorImageInfo> obj) {
		for (auto& descBind : bindings) {
			if (descBind.binding == binding) {
				VkWriteDescriptorSetAccelerationStructureKHR writeDescAS{};
				writeDescAS.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
				writeDescAS.accelerationStructureCount = 1;

				VkWriteDescriptorSet descSetWrite{};
				descSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descSetWrite.descriptorCount = 1;
				descSetWrite.descriptorType = descBind.descriptorType;
				descSetWrite.dstBinding = descBind.binding;
				descSetWrite.dstSet = descSet[index];
				VkDescriptorImageInfo descInfo;
				switch(descBind.descriptorType) {
				case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
					writeDescAS.pAccelerationStructures = &std::get<VkAccelerationStructureKHR>(obj);
					descSetWrite.pNext = &writeDescAS;
					break;
				case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
					descInfo = std::get<VkDescriptorImageInfo>(obj);
					descSetWrite.pImageInfo = &descInfo;
					break;
				}
				vkUpdateDescriptorSets(ctx->vkDevice(), 1, &descSetWrite, 0, nullptr);
			}
		}
	}

#define CHECK_OUT_OF_INDEX(x) \
if (index >= commandBufferList.size()) {\
	LOG_ERROR(L"Command Buffer Index out of range in %s", CString(__FUNCTION__));\
	return x;\
}

	VkCommandBufferManager::VkCommandBufferManager(const VkContext* ctx) : vkctx(ctx), commandBufferList() {}

	void VkCommandBufferManager::allocate(uint32_t count) {
		VkCommandBufferAllocateInfo commandBufferAI{};
		commandBufferAI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAI.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAI.commandPool = vkctx->vkCommandPool();
		commandBufferAI.commandBufferCount = count;
		commandBufferList.resize(count);
		vkAllocateCommandBuffers(vkctx->vkDevice(), &commandBufferAI, commandBufferList.data());
	}

	VkCommandBuffer VkCommandBufferManager::get(uint32_t index) const {
		CHECK_OUT_OF_INDEX(VK_NULL_HANDLE)
			return commandBufferList.at(index);
	}

	VkResult VkCommandBufferManager::submit(uint32_t index) {
		CHECK_OUT_OF_INDEX(VK_ERROR_UNKNOWN);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBufferList.at(index);
		return vkQueueSubmit(vkctx->vkQueue(), 1, &submitInfo, VK_NULL_HANDLE);
	}

	VkResult VkCommandBufferManager::begin(uint32_t index) {
		CHECK_OUT_OF_INDEX(VK_ERROR_UNKNOWN);
		VkCommandBufferBeginInfo commandBufferBI{};
		commandBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		return vkBeginCommandBuffer(commandBufferList[index], &commandBufferBI);
	}

	VkResult VkCommandBufferManager::end(uint32_t index) {
		CHECK_OUT_OF_INDEX(VK_ERROR_UNKNOWN);
		return vkEndCommandBuffer(commandBufferList[index]);
	}

	void VkCommandBufferManager::free(uint32_t count) {
		count = std::min(count, static_cast<uint32_t>(commandBufferList.size()));
		vkFreeCommandBuffers(vkctx->vkDevice(), vkctx->vkCommandPool(), count, commandBufferList.data());
		commandBufferList.erase(commandBufferList.begin(), commandBufferList.begin() + count);
	}

	VkShaderModule loadShaderModule(const VkContext* ctx, std::string fileName) {
		std::ifstream shaderFile(fileName, std::ios::ate | std::ios::binary | std::ios::in);
		if (!shaderFile.is_open()) {
			LOG_ERROR(L"Couldn't Open Shader File: %s", CString(fileName.c_str()));
			return VK_NULL_HANDLE;
		}
		size_t codeSize = shaderFile.tellg();
		shaderFile.seekg(std::ios::beg);
		char* shaderCode = new char[codeSize];
		shaderFile.read(shaderCode, codeSize);
		VkShaderModuleCreateInfo shaderModuleCI{};
		shaderModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderModuleCI.codeSize = codeSize;
		shaderModuleCI.pCode = reinterpret_cast<uint32_t*>(shaderCode);
		VkShaderModule shaderModule;
		CHECK_VULKAN_ERROR(vkCreateShaderModule(ctx->vkDevice(), &shaderModuleCI, nullptr, &shaderModule));
		delete[] shaderCode;
		return shaderModule;
	}
}