#include "XrEngine.h"

#include <cstring>

XrEngine::XrEngine()
{
}

XrEngine::~XrEngine()
{
}

bool XrEngine::bindFunctions()
{
	XrResult result = XR_SUCCESS;

    result = xrGetInstanceProcAddr(instance, "xrGetVulkanInstanceExtensionsKHR", reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetVulkanInstanceExtensionsKHR));
	if (result != XR_SUCCESS)
	{
		Logger::print(TinyEngine_ERROR, __FILE__, __LINE__, "OpenXR");

		return false;
	}

    result = xrGetInstanceProcAddr(instance, "xrGetVulkanDeviceExtensionsKHR", reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetVulkanDeviceExtensionsKHR));
	if (result != XR_SUCCESS)
	{
		Logger::print(TinyEngine_ERROR, __FILE__, __LINE__, "OpenXR");

		return false;
	}

    result = xrGetInstanceProcAddr(instance, "xrGetVulkanGraphicsDeviceKHR", reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetVulkanGraphicsDeviceKHR));
	if (result != XR_SUCCESS)
	{
		Logger::print(TinyEngine_ERROR, __FILE__, __LINE__, "OpenXR");

		return false;
	}

    result = xrGetInstanceProcAddr(instance, "xrGetVulkanGraphicsRequirementsKHR", reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetVulkanGraphicsRequirementsKHR));
	if (result != XR_SUCCESS)
	{
		Logger::print(TinyEngine_ERROR, __FILE__, __LINE__, "OpenXR");

		return false;
	}

	return true;
}

bool XrEngine::prepare()
{
	return true;
}

bool XrEngine::init(VkInstance vulkanInstance, VkPhysicalDevice vulkanPhysicalDevice, VkDevice vulkanDevice, uint32_t vulkanQueueFamilyIndex, uint32_t vulkanQueueIndex, VkFormat vulkanFormat)
{
	std::vector<const char*> enabledInstanceExtensionNames;
	enabledInstanceExtensionNames.push_back(XR_KHR_VULKAN_ENABLE_EXTENSION_NAME);

	//

	XrResult result = XR_SUCCESS;

    XrInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.type = XR_TYPE_INSTANCE_CREATE_INFO;

    instanceCreateInfo.applicationInfo = {};
    strncpy(instanceCreateInfo.applicationInfo.applicationName, "Hello OpenXR", XR_MAX_APPLICATION_NAME_SIZE);
    instanceCreateInfo.applicationInfo.applicationVersion = 1;
    strncpy(instanceCreateInfo.applicationInfo.engineName, "XrEngine", XR_MAX_ENGINE_NAME_SIZE);
    instanceCreateInfo.applicationInfo.engineVersion = 1;
    instanceCreateInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(enabledInstanceExtensionNames.size());
	instanceCreateInfo.enabledExtensionNames = enabledInstanceExtensionNames.data();

    result = xrCreateInstance(&instanceCreateInfo, &instance);
	if (result != XR_SUCCESS)
	{
		Logger::print(TinyEngine_ERROR, __FILE__, __LINE__, "OpenXR");

		return false;
	}

	//

	if (!bindFunctions())
	{
		return false;
	}

	//

    XrSystemGetInfo systemGetInfo = {};
    systemGetInfo.type = XR_TYPE_SYSTEM_GET_INFO;
    systemGetInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

    result = xrGetSystem(instance, &systemGetInfo, &systemId);
	if (result != XR_SUCCESS)
	{
		Logger::print(TinyEngine_ERROR, __FILE__, __LINE__, "OpenXR");

		return false;
	}

    XrSystemProperties systemProperties = {};
    systemProperties.type = XR_TYPE_SYSTEM_PROPERTIES;

    result = xrGetSystemProperties(instance, systemId, &systemProperties);
	if (result != XR_SUCCESS)
	{
		Logger::print(TinyEngine_ERROR, __FILE__, __LINE__, "OpenXR");

		return false;
	}

	Logger::print(TinyEngine_INFO, __FILE__, __LINE__, "System Name: '%s'", systemProperties.systemName);

	//

	uint32_t environmentBlendModeCount = 0;
	result = xrEnumerateEnvironmentBlendModes(instance, systemId, viewConfigurationType, 0, &environmentBlendModeCount, nullptr);
	if (result != XR_SUCCESS || environmentBlendModeCount == 0)
	{
		Logger::print(TinyEngine_ERROR, __FILE__, __LINE__, "OpenXR");

		return false;
	}

	std::vector<XrEnvironmentBlendMode> environmentBlendModes(environmentBlendModeCount);
	result = xrEnumerateEnvironmentBlendModes(instance, systemId, viewConfigurationType, environmentBlendModeCount, &environmentBlendModeCount, environmentBlendModes.data());
	if (result != XR_SUCCESS)
	{
		Logger::print(TinyEngine_ERROR, __FILE__, __LINE__, "OpenXR");

		return false;
	}

	environmentBlendMode = environmentBlendModes[0];

	// Vulkan specific

	XrGraphicsRequirementsVulkanKHR graphicsRequirementsVulkan = {};
	graphicsRequirementsVulkan.type = XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR;

	result = pfnGetVulkanGraphicsRequirementsKHR(instance, systemId, &graphicsRequirementsVulkan);
	if (result != XR_SUCCESS)
	{
		Logger::print(TinyEngine_ERROR, __FILE__, __LINE__, "OpenXR");

		return false;
	}

	XrGraphicsBindingVulkanKHR graphicsBindingVulkan = {};
	graphicsBindingVulkan.type = XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR;
	graphicsBindingVulkan.instance = vulkanInstance;
	graphicsBindingVulkan.physicalDevice = vulkanPhysicalDevice;
	graphicsBindingVulkan.device = vulkanDevice;
	graphicsBindingVulkan.queueFamilyIndex = vulkanQueueFamilyIndex;
	graphicsBindingVulkan.queueIndex = vulkanQueueIndex;

	//

    XrSessionCreateInfo sessionCreateInfo = {};
    sessionCreateInfo.type = XR_TYPE_SESSION_CREATE_INFO;
    sessionCreateInfo.next = &graphicsBindingVulkan;
    sessionCreateInfo.systemId = systemId;
    result = xrCreateSession(instance, &sessionCreateInfo, &session);
	if (result != XR_SUCCESS)
	{
		Logger::print(TinyEngine_ERROR, __FILE__, __LINE__, "OpenXR");

		return false;
	}

	//

	uint32_t formatCount = 0;
	result = xrEnumerateSwapchainFormats(session, 0, &formatCount, nullptr);
	if (result != XR_SUCCESS || formatCount == 0)
	{
		Logger::print(TinyEngine_ERROR, __FILE__, __LINE__, "OpenXR");

		return false;
	}

    std::vector<int64_t> formats(formatCount);
    result = xrEnumerateSwapchainFormats(session, formatCount, &formatCount, formats.data());
	if (result != XR_SUCCESS)
	{
		Logger::print(TinyEngine_ERROR, __FILE__, __LINE__, "OpenXR");

		return false;
	}

	int64_t swapchainFormat = 0;
	for (int64_t currentFormat : formats)
	{
		if ((VkFormat)currentFormat == vulkanFormat)
		{
			swapchainFormat = currentFormat;
			break;
		}
	}

	if (swapchainFormat == 0)
	{
		return false;
	}

	//

    uint32_t viewCount = 0;
    result = xrEnumerateViewConfigurationViews(instance, systemId, viewConfigurationType, 0, &viewCount, nullptr);
	if (result != XR_SUCCESS || viewCount == 0)
	{
		Logger::print(TinyEngine_ERROR, __FILE__, __LINE__, "OpenXR");

		return false;
	}

	std::vector<XrViewConfigurationView> viewConfigurationView(viewCount);
	result = xrEnumerateViewConfigurationViews(instance, systemId, viewConfigurationType, viewCount, &viewCount, viewConfigurationView.data());
	if (result != XR_SUCCESS)
	{
		Logger::print(TinyEngine_ERROR, __FILE__, __LINE__, "OpenXR");

		return false;
	}

	swapchains.resize(viewCount);

	for (uint32_t i = 0; i < viewCount; i++)
	{
	    XrSwapchainCreateInfo swapchainCreateInfo = {};
	    swapchainCreateInfo.type = XR_TYPE_SWAPCHAIN_CREATE_INFO;
	    swapchainCreateInfo.arraySize = 1;
	    swapchainCreateInfo.format = swapchainFormat;
	    swapchainCreateInfo.width = viewConfigurationView[i].recommendedImageRectWidth;
	    swapchainCreateInfo.height = viewConfigurationView[i].recommendedImageRectHeight;
	    swapchainCreateInfo.mipCount = 1;
	    swapchainCreateInfo.faceCount = 1;
	    swapchainCreateInfo.sampleCount = viewConfigurationView[i].recommendedSwapchainSampleCount;
	    swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;

	    result = xrCreateSwapchain(session, &swapchainCreateInfo, &swapchains[i]);
		if (result != XR_SUCCESS)
		{
			Logger::print(TinyEngine_ERROR, __FILE__, __LINE__, "OpenXR");

			return false;
		}
	}

	//

    XrReferenceSpaceCreateInfo referenceSpaceCreateInfo = {};
    referenceSpaceCreateInfo.type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO;

    referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
    referenceSpaceCreateInfo.poseInReferenceSpace.orientation = {0.0f, 0.0f, 0.0f, 1.0f};
    referenceSpaceCreateInfo.poseInReferenceSpace.position = {0.0f, 0.0f, 0.0f};

    result = xrCreateReferenceSpace(session, &referenceSpaceCreateInfo, &space);
	if (result != XR_SUCCESS)
	{
		Logger::print(TinyEngine_ERROR, __FILE__, __LINE__, "OpenXR");

		return false;
	}

	return true;
}

bool XrEngine::resize()
{
	return true;
}

bool XrEngine::update()
{
	XrResult result = XR_SUCCESS;

	//

	do {
		XrEventDataBuffer eventDataBuffer = {};
		eventDataBuffer.type = XR_TYPE_EVENT_DATA_BUFFER;

		result = xrPollEvent(instance, &eventDataBuffer);
		if (result == XR_SUCCESS)
		{
			switch (eventDataBuffer.type)
			{
				case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
				{
					const XrEventDataSessionStateChanged eventDataSessionStateChanged = *reinterpret_cast<const XrEventDataSessionStateChanged*>(&eventDataBuffer);

					sessionState = eventDataSessionStateChanged.state;
					switch (sessionState)
					{
						case XR_SESSION_STATE_IDLE:
						{
							Logger::print(TinyEngine_INFO, __FILE__, __LINE__, "OpenXR Session State Idle");

							break;
						}
						case XR_SESSION_STATE_READY:
						{
							Logger::print(TinyEngine_INFO, __FILE__, __LINE__, "OpenXR Session State Ready");

							XrSessionBeginInfo sessionBeginInfo = {};
							sessionBeginInfo.type = XR_TYPE_SESSION_BEGIN_INFO;
							sessionBeginInfo.primaryViewConfigurationType = viewConfigurationType;
							result = xrBeginSession(session, &sessionBeginInfo);
							if (result != XR_SUCCESS)
							{
								Logger::print(TinyEngine_ERROR, __FILE__, __LINE__, "OpenXR");

								return false;
							}

							sessionRunning = true;

							break;
						}
						case XR_SESSION_STATE_SYNCHRONIZED:
						{
							Logger::print(TinyEngine_INFO, __FILE__, __LINE__, "OpenXR Session State Synchronized");

							break;
						}
						case XR_SESSION_STATE_VISIBLE:
						{
							Logger::print(TinyEngine_INFO, __FILE__, __LINE__, "OpenXR Session State Visible");

							break;
						}
						case XR_SESSION_STATE_FOCUSED:
						{
							Logger::print(TinyEngine_INFO, __FILE__, __LINE__, "OpenXR Session State Focused");

							break;
						}
						case XR_SESSION_STATE_STOPPING:
						{
							Logger::print(TinyEngine_INFO, __FILE__, __LINE__, "OpenXR Session State Stopping");

							result = xrEndSession(session);
							if (result != XR_SUCCESS)
							{
								Logger::print(TinyEngine_ERROR, __FILE__, __LINE__, "OpenXR");

								return false;
							}

							sessionRunning = false;

							break;
						}
						case XR_SESSION_STATE_LOSS_PENDING:
						{
							Logger::print(TinyEngine_INFO, __FILE__, __LINE__, "OpenXR Session State Loss Pending");

							break;
						}
						case XR_SESSION_STATE_EXITING:
						{
							Logger::print(TinyEngine_INFO, __FILE__, __LINE__, "OpenXR Session State Exiting");

							break;
						}
						default:
							Logger::print(TinyEngine_ERROR, __FILE__, __LINE__, "OpenXR");
							return false;
					}

					break;
				}
				default:
					Logger::print(TinyEngine_INFO, __FILE__, __LINE__, "OpenXR Event Type: %u", eventDataBuffer.type);
			}
		}
		else if (result != XR_EVENT_UNAVAILABLE)
		{
			Logger::print(TinyEngine_ERROR, __FILE__, __LINE__, "OpenXR");

			return false;
		}
	} while (result == XR_SUCCESS);

	//

	if (!sessionRunning)
	{
		return true;
	}

	//

	XrFrameWaitInfo frameWaitInfo = {};
	frameWaitInfo.type = XR_TYPE_FRAME_WAIT_INFO;

	XrFrameState frameState = {};
	frameState.type = XR_TYPE_FRAME_STATE;

	result = xrWaitFrame(session, &frameWaitInfo, &frameState);
	if (result != XR_SUCCESS)
	{
		Logger::print(TinyEngine_ERROR, __FILE__, __LINE__, "OpenXR");

		return false;
	}

    XrFrameBeginInfo frameBeginInfo = {};
    frameBeginInfo.type = XR_TYPE_FRAME_BEGIN_INFO;

    result = xrBeginFrame(session, &frameBeginInfo);
	if (result != XR_SUCCESS)
	{
		Logger::print(TinyEngine_ERROR, __FILE__, __LINE__, "OpenXR");

		return false;
	}

    std::vector<XrCompositionLayerBaseHeader*> layers;

    if (frameState.shouldRender)
    {
    	XrCompositionLayerProjection compositionLayerProjection = {};
    	compositionLayerProjection.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION;

    	// TODO: Implement.
    }

    XrFrameEndInfo frameEndInfo = {};
    frameEndInfo.type = XR_TYPE_FRAME_END_INFO;
    frameEndInfo.displayTime = frameState.predictedDisplayTime;
    frameEndInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    frameEndInfo.layerCount = static_cast<uint32_t>(layers.size());
    frameEndInfo.layers = layers.data();

    result = xrEndFrame(session, &frameEndInfo);
	if (result != XR_SUCCESS)
	{
		Logger::print(TinyEngine_ERROR, __FILE__, __LINE__, "OpenXR");

		return false;
	}

	return true;
}

bool XrEngine::terminate()
{
	if (space != XR_NULL_HANDLE)
	{
		xrDestroySpace(space);
		space = XR_NULL_HANDLE;
	}

	for (XrSwapchain swapchain : swapchains)
	{
		xrDestroySwapchain(swapchain);
	}
	swapchains.clear();

	if (session != XR_NULL_HANDLE)
	{
		xrDestroySession(session);
		session = XR_NULL_HANDLE;
	}
	sessionState = XR_SESSION_STATE_UNKNOWN;
	bool sessionRunning = false;

	environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;

	viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

	systemId = XR_NULL_SYSTEM_ID;

	if (instance != XR_NULL_HANDLE)
	{
		xrDestroyInstance(instance);
		instance = XR_NULL_HANDLE;
	}

	return true;
}