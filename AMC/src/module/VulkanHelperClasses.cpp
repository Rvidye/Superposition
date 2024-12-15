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
	}

	VkContext::~VkContext() {
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

	VkDescSetManager::Builder::Builder(const VkContext* context) {
		descSetData = {};
		ctx = context;
	}

	VkDescSetManager::Builder& VkDescSetManager::Builder::addToDescSet(std::string key, uint32_t binding, VkDescriptorType descType, VkShaderStageFlags stageFlags, std::vector<VkBuffer> bufferVector) {
		if (descSetData.find(key) != descSetData.end() && descSetData[key].size() > 0) {
			if (descSetData[key][0].resourceVec.size() != bufferVector.size()) {
				LOG_ERROR(L"Descriptor Set Resource Count Mismatch. Recived %d resource, but expecting %d.", bufferVector.size(), descSetData[key][0].resourceVec.size());
				return *this;
			}
			for (const auto& data : descSetData[key]) {
				if (binding == data.binding) {
					LOG_ERROR(L"Binding %d already present in '%s' Descriptor Set", binding, CString(key.c_str()));
					return *this;
				}
			}
		}
		descSetData[key].push_back(DescriptorSetData(binding, descType, stageFlags, bufferVector));
		return *this;
	}

	VkDescSetManager* VkDescSetManager::Builder::build() {
		VkDescSetManager* descSetMan = new VkDescSetManager();

		VkDescriptorSetLayoutCreateInfo descSetLayoutCI{};
		descSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		std::unordered_map<VkDescriptorType, size_t> poolSizeMap{};
		size_t maxSet = 0;
		std::unordered_map<std::string, size_t> maxCountPerLayout;

		for (auto& entry : descSetData) {
			descSetMan->descriptorSet[entry.first] = {};

			std::vector<VkDescriptorSetLayoutBinding> bindings{};

			size_t temp = 0;
			for (auto& bindingData : entry.second) {
				VkDescriptorSetLayoutBinding descSetBinding{};
				descSetBinding.binding = bindingData.binding;
				descSetBinding.descriptorCount = 1;
				descSetBinding.descriptorType = bindingData.type;
				descSetBinding.stageFlags = bindingData.shaderStages;

				poolSizeMap[bindingData.type] += bindingData.resourceVec.size();

				bindings.push_back(descSetBinding);

				temp = bindingData.resourceVec.size();
			}
			maxCountPerLayout[entry.first] = temp;
			maxSet += temp;

			descSetLayoutCI.bindingCount = static_cast<uint32_t>(bindings.size());
			descSetLayoutCI.pBindings = bindings.data();

			vkCreateDescriptorSetLayout(ctx->vkDevice(), &descSetLayoutCI, nullptr, &descSetMan->descriptorSet[entry.first].first);
		}

		std::vector<VkDescriptorPoolSize> poolSizes{};
		for (auto& poolEntry : poolSizeMap) {
			poolSizes.push_back({ poolEntry.first, static_cast<uint32_t>(poolEntry.second) });
		}

		VkDescriptorPoolCreateInfo descPoolCI{};
		descPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descPoolCI.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		descPoolCI.pPoolSizes = poolSizes.data();
		descPoolCI.maxSets = static_cast<uint32_t>(maxSet);
		vkCreateDescriptorPool(ctx->vkDevice(), &descPoolCI, nullptr, &descSetMan->pool);

		VkDescriptorSetAllocateInfo descSetAI{};
		descSetAI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descSetAI.descriptorPool = descSetMan->pool;

		VkWriteDescriptorSet writeDescSet{};
		writeDescSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescSet.descriptorCount = 1;
		writeDescSet.dstArrayElement = 0;

		VkDescriptorBufferInfo descBufferInfo{};
		descBufferInfo.offset = 0;
		descBufferInfo.range = VK_WHOLE_SIZE;

		for (auto& entry : descSetMan->descriptorSet) {
			std::vector<VkDescriptorSetLayout> layoutsArray(maxCountPerLayout[entry.first], entry.second.first);

			descSetAI.descriptorSetCount = static_cast<uint32_t>(layoutsArray.size());
			descSetAI.pSetLayouts = layoutsArray.data();
			descSetMan->descriptorSet[entry.first].second.resize(descSetAI.descriptorSetCount);
			vkAllocateDescriptorSets(ctx->vkDevice(), &descSetAI, descSetMan->descriptorSet[entry.first].second.data());

			for (auto& layoutBinding : descSetData[entry.first]) {
				writeDescSet.descriptorType = layoutBinding.type;
				writeDescSet.dstBinding = layoutBinding.binding;

				for (uint32_t i = 0; i < maxCountPerLayout[entry.first]; i++) {
					writeDescSet.dstSet = descSetMan->descriptorSet[entry.first].second[i];
					switch (layoutBinding.type) {
					case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
						writeDescSet.pBufferInfo = &descBufferInfo;
						descBufferInfo.buffer = std::get<VkBuffer>(layoutBinding.resourceVec[i]);
						break;
					case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
						break;
					}
					vkUpdateDescriptorSets(ctx->vkDevice(), 1, &writeDescSet, 0, nullptr);
				}
			}
		}

		return descSetMan;
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