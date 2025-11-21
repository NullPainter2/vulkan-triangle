#include "assert.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include "File.hpp"
#include <vector>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

namespace VK
{
	struct MyVulkan {
		char* layerNames[1] = { "VK_LAYER_KHRONOS_validation" };
		int layerNamesCount = 1;
		std::vector<char*> instanceExtensionNames = std::vector<char*>();
		std::vector<char*> deviceExtensionNames = std::vector<char*>();

		int WIDTH = 800;
		int HEIGHT = 600;

		VkPipeline pipeline = {};

		HWND window = NULL;

		int graphicQueIndex = -1;
		int presentQueIndex = -1;
		VkCommandBuffer commandBuffer;
		VkSwapchainKHR swapChain = NULL;
		VkDevice device = NULL;
		VkFence renderingFrameFence = NULL;
		VkSemaphore frameReadySemaphore = {};
		VkRenderPass renderPass = {};
		VkFramebuffer* frameBuffers = NULL;
		VkSurfaceCapabilitiesKHR caps = {};

	};

	VkDevice _CreateLogicalDevice(MyVulkan*,VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, int* graphicQueIndex, int* presentQueIndex);
	VkPhysicalDevice _ChoosePhysicalDevice(VkInstance instance);
	VkInstance _CreateInstance(MyVulkan*);
	//void DrawCall(uint32_t frameBufferIndex);


	// auto extensionNames = std::vector<char*>();
	//char * extensionNames[] =  {""};
	//int extensionNamesCount = 0;

#pragma region procedures loaded from .dll
#define vk_fun(name) PFN_##name name;
	vk_fun(vkGetInstanceProcAddr);
	vk_fun(vkCreateWin32SurfaceKHR);
	vk_fun(vkCreateInstance);
	vk_fun(vkEnumerateInstanceExtensionProperties);
	vk_fun(vkEnumerateInstanceLayerProperties);
	vk_fun(vkEnumeratePhysicalDevices);
	vk_fun(vkGetPhysicalDeviceProperties);
	vk_fun(vkGetPhysicalDeviceFeatures);
	vk_fun(vkGetPhysicalDeviceQueueFamilyProperties);
	vk_fun(vkCreateDevice);
	vk_fun(vkEnumerateDeviceExtensionProperties);
	vk_fun(vkCreateDebugUtilsMessengerEXT);
	vk_fun(vkGetPhysicalDeviceSurfaceSupportKHR);
	vk_fun(vkCreateSwapchainKHR);
	vk_fun(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
	vk_fun(vkGetPhysicalDeviceSurfaceFormatsKHR);
	vk_fun(vkGetPhysicalDeviceSurfacePresentModesKHR);
	vk_fun(vkCreateShaderModule);
	vk_fun(vkCreatePipelineCache);
	vk_fun(vkCreateGraphicsPipelines);
	vk_fun(vkBeginCommandBuffer);
	vk_fun(vkCmdBindPipeline);
	vk_fun(vkCmdDraw);
	vk_fun(vkQueuePresentKHR);
	vk_fun(vkCmdBindShadersEXT);
	vk_fun(vkEndCommandBuffer);
	vk_fun(vkCreatePipelineLayout);
	vk_fun(vkCreateRenderPass);
	vk_fun(vkCreateCommandPool);
	vk_fun(vkAllocateCommandBuffers);
	vk_fun(vkCmdBeginRenderPass);
	vk_fun(vkCreateFramebuffer);
	vk_fun(vkCreateImageView);
	vk_fun(vkCreateImage);
	vk_fun(vkGetSwapchainImagesKHR);
	vk_fun(vkCmdEndRenderPass);
	vk_fun(vkQueueSubmit);
	vk_fun(vkCreateFence);
	vk_fun(vkDestroyFence);
	vk_fun(vkWaitForFences);
	vk_fun(vkGetDeviceQueue);
	vk_fun(vkResetFences);
	vk_fun(vkCreateSemaphore);
	vk_fun(vkAcquireNextImageKHR);
	vk_fun(vkCmdSetViewport);
	vk_fun(vkResetCommandBuffer);
	vk_fun(vkCmdSetScissor);

#pragma endregion

	void* _LoadProcedure(char* dllName, char* procName)
	{
		auto dll = LoadLibraryA(dllName);
		assert(dll);
		void* result = GetProcAddress(dll, procName);
		assert(result);
		return result;
	}

	VkBool32 OnDebugReportCallbackEXT(
		VkDebugReportFlagsEXT                       flags,
		VkDebugReportObjectTypeEXT                  objectType,
		uint64_t                                    object,
		size_t                                      location,
		int32_t                                     messageCode,
		const char* pLayerPrefix,
		const char* pMessage,
		void* pUserData
	)
	{
		printf("DEBUG: [%d] '%s'\n", flags, pMessage ? pMessage : "???");
		return VK_FALSE; // required
	}

	VkBool32 OnDebugUtilsMessanger(
		VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		assert(pCallbackData);
		printf("DEBUG: [%s] '%s'\n", pCallbackData->pMessageIdName, pCallbackData->pMessage);
		return VK_FALSE;
	}

	bool quitFromMessageHandler = false;

	LRESULT CALLBACK
		_OnWindowMessage(HWND window, UINT message_id, WPARAM wparam, LPARAM lparam)
	{
		if (message_id == WM_SIZE)
		{
			//GPU::win32_resizeScreenBuffer( window );

			return 0;
		}
		else if (message_id == WM_CLOSE)
		{
			quitFromMessageHandler = true;

			return 0;
		}
		else
		{
			return DefWindowProcA(window, message_id, wparam, lparam);
		}
	}

	VkSurfaceKHR _CreateWindowSurface(MyVulkan* my, VkInstance instance)
	{
		// https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/01_Presentation/00_Window_surface.html

		WNDCLASSA windowClass = {};
		windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		windowClass.lpszClassName = "my-window-class";
		windowClass.lpfnWndProc = _OnWindowMessage;
		windowClass.hInstance = GetModuleHandle(nullptr);
		assert(RegisterClassA(&windowClass));
		my->window = CreateWindowExA(0, windowClass.lpszClassName, "Vulkan Triangle!", WS_TILEDWINDOW | WS_VISIBLE, 0, 0, my->WIDTH, my->HEIGHT, NULL, NULL, NULL, NULL);
		assert(my->window);
		VkWin32SurfaceCreateInfoKHR createInfo32 = {};
		createInfo32.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		createInfo32.hwnd = my->window;
		createInfo32.hinstance = windowClass.hInstance;
		VkSurfaceKHR surface = NULL;
		assert(vkCreateWin32SurfaceKHR(instance, &createInfo32, NULL, &surface) == VK_SUCCESS);
		return surface;
	}

	void _InstallDebugCallbacks(VkInstance instance)
	{
		VkDebugUtilsMessengerCreateInfoEXT debug1 = {};
		debug1.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debug1.messageSeverity = (
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
			//VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
			);
		debug1.messageType = (
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
			// VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT
			);
		debug1.pfnUserCallback = OnDebugUtilsMessanger;

		VkDebugUtilsMessengerEXT debugHandle;
		assert(vkCreateDebugUtilsMessengerEXT(instance, &debug1, NULL, &debugHandle) == VK_SUCCESS);
	}

	void _CreatePresentationQue(VkPhysicalDevice physicalDevice, uint32_t graphicFamilyIndex, VkSurfaceKHR surface)
	{
	}

	void _CreateSwapChain(MyVulkan* my, VkSurfaceKHR surface, VkPhysicalDevice physicalDevice)
	{
		VkSwapchainCreateInfoKHR swapInfo = {};
		swapInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapInfo.surface = surface;
		// swapInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		uint32_t formatsCount = 0;
		VkSurfaceFormatKHR* formats = NULL;
		assert(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatsCount, NULL) == VK_SUCCESS);
		formats = (VkSurfaceFormatKHR*)calloc(formatsCount, sizeof(formats[0]));
		assert(formats);
		assert(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatsCount, formats) == VK_SUCCESS);
		printf("[INFO] format count %d\n", formatsCount);
		int bestFormatIndex = -1;
		for (int i = 0; i < formatsCount; i++)
		{
			if (formats[i].colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR && formats[i].format == VK_FORMAT_B8G8R8A8_SRGB) //  VK_FORMAT_R8G8B8_SRGB ?
			{
				// TODO: print those
				bestFormatIndex = i;
				break;
			}
		}
		assert(bestFormatIndex >= 0);

		VkPresentModeKHR presentMode;
		VkPresentModeKHR* presents;
		uint32_t presentsCount;
		assert(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentsCount, NULL) == VK_SUCCESS);
		printf("[INFO] present modes count %d\n", presentsCount);
		assert(presentsCount > 0);
		presents = (VkPresentModeKHR*)calloc(presentsCount, sizeof(presents[0]));
		assert(presents);
		assert(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentsCount, presents) == VK_SUCCESS);

		assert(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &my->caps) == VK_SUCCESS);
		swapInfo.minImageCount = my->caps.minImageCount;
		swapInfo.imageFormat = formats[bestFormatIndex].format; // VK_FORMAT_B8G8R8A8_SRGB
		swapInfo.imageColorSpace = formats[bestFormatIndex].colorSpace;
		swapInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapInfo.preTransform = my->caps.currentTransform;
		swapInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapInfo.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR; // TODO: make sure it is in `presents` array
		swapInfo.imageArrayLayers = 1; // not stereoscopic 3d
		VkExtent2D extent = {};
		extent.width = my->caps.minImageExtent.width;//WIDTH;
		extent.height = my->caps.minImageExtent.height;//HEIGHT;
		swapInfo.imageExtent = extent;
		swapInfo.clipped = VK_TRUE; // tutorial
		VkResult result = vkCreateSwapchainKHR(my->device, &swapInfo, NULL, &my->swapChain);
		assert(result == VK_SUCCESS);
	}

	int getWidth(int width, VkSurfaceCapabilitiesKHR caps)
	{
		if (width > caps.maxImageExtent.width) return caps.maxImageExtent.width;
		if (width < caps.minImageExtent.width) return caps.minImageExtent.width;
		return width;
	}

	int getHeight(int height, VkSurfaceCapabilitiesKHR caps)
	{
		if (height > caps.maxImageExtent.height) return caps.maxImageExtent.height;
		if (height < caps.minImageExtent.height) return caps.minImageExtent.height;
		return height;
	}

	void Init(MyVulkan* my)
	{
#pragma region "load vulkan libraries from dll"

		// get loader https://docs.vulkan.org/guide/latest/loader.html
		vkCreateInstance = (PFN_vkCreateInstance)_LoadProcedure("vulkan-1.dll", "vkCreateInstance");
		vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)_LoadProcedure("vulkan-1.dll", "vkGetInstanceProcAddr");
		vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)_LoadProcedure("vulkan-1.dll", "vkCreateWin32SurfaceKHR");
		vkEnumerateInstanceLayerProperties = (PFN_vkEnumerateInstanceLayerProperties)_LoadProcedure("vulkan-1.dll", "vkEnumerateInstanceLayerProperties");
		vkEnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices)_LoadProcedure("vulkan-1.dll", "vkEnumeratePhysicalDevices");
		// device
		vkGetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties)_LoadProcedure("vulkan-1.dll", "vkGetPhysicalDeviceProperties");
		vkGetPhysicalDeviceQueueFamilyProperties = (PFN_vkGetPhysicalDeviceQueueFamilyProperties)_LoadProcedure("vulkan-1.dll", "vkGetPhysicalDeviceQueueFamilyProperties");
		vkGetPhysicalDeviceFeatures = (PFN_vkGetPhysicalDeviceFeatures)_LoadProcedure("vulkan-1.dll", "vkGetPhysicalDeviceFeatures");
		vkCreateDevice = (PFN_vkCreateDevice)_LoadProcedure("vulkan-1.dll", "vkCreateDevice");
		vkEnumerateDeviceExtensionProperties = (PFN_vkEnumerateDeviceExtensionProperties)_LoadProcedure("vulkan-1.dll", "vkEnumerateDeviceExtensionProperties");
		vkGetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR)_LoadProcedure("vulkan-1.dll", "vkGetPhysicalDeviceSurfaceSupportKHR");
		vkCreateSwapchainKHR = (PFN_vkCreateSwapchainKHR)_LoadProcedure("vulkan-1.dll", "vkCreateSwapchainKHR");
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)_LoadProcedure("vulkan-1.dll", "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
		vkGetPhysicalDeviceSurfaceFormatsKHR = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)_LoadProcedure("vulkan-1.dll", "vkGetPhysicalDeviceSurfaceFormatsKHR");
		vkGetPhysicalDeviceSurfacePresentModesKHR = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)_LoadProcedure("vulkan-1.dll", "vkGetPhysicalDeviceSurfacePresentModesKHR");
		vkCreateShaderModule = (PFN_vkCreateShaderModule)_LoadProcedure("vulkan-1.dll", "vkCreateShaderModule");
		vkCreatePipelineCache = (PFN_vkCreatePipelineCache)_LoadProcedure("vulkan-1.dll", "vkCreatePipelineCache");
		vkCreateGraphicsPipelines = (PFN_vkCreateGraphicsPipelines)_LoadProcedure("vulkan-1.dll", "vkCreateGraphicsPipelines");
		vkBeginCommandBuffer = (PFN_vkBeginCommandBuffer)_LoadProcedure("vulkan-1.dll", "vkBeginCommandBuffer");
		vkCmdBindPipeline = (PFN_vkCmdBindPipeline)_LoadProcedure("vulkan-1.dll", "vkCmdBindPipeline");
		vkCmdDraw = (PFN_vkCmdDraw)_LoadProcedure("vulkan-1.dll", "vkCmdDraw");
		vkQueuePresentKHR = (PFN_vkQueuePresentKHR)_LoadProcedure("vulkan-1.dll", "vkQueuePresentKHR");
		vkEndCommandBuffer = (PFN_vkEndCommandBuffer)_LoadProcedure("vulkan-1.dll", "vkEndCommandBuffer");
		vkCreatePipelineLayout = (PFN_vkCreatePipelineLayout)_LoadProcedure("vulkan-1.dll", "vkCreatePipelineLayout");
		vkCreateRenderPass = (PFN_vkCreateRenderPass)_LoadProcedure("vulkan-1.dll", "vkCreateRenderPass");
		vkCreateCommandPool = (PFN_vkCreateCommandPool)_LoadProcedure("vulkan-1.dll", "vkCreateCommandPool");
		vkAllocateCommandBuffers = (PFN_vkAllocateCommandBuffers)_LoadProcedure("vulkan-1.dll", "vkAllocateCommandBuffers");
		vkCmdBeginRenderPass = (PFN_vkCmdBeginRenderPass)_LoadProcedure("vulkan-1.dll", "vkCmdBeginRenderPass");
		vkCreateFramebuffer = (PFN_vkCreateFramebuffer)_LoadProcedure("vulkan-1.dll", "vkCreateFramebuffer");
		vkCreateImageView = (PFN_vkCreateImageView)_LoadProcedure("vulkan-1.dll", "vkCreateImageView");
		vkCreateImage = (PFN_vkCreateImage)_LoadProcedure("vulkan-1.dll", "vkCreateImage");
		vkGetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)_LoadProcedure("vulkan-1.dll", "vkGetSwapchainImagesKHR");
		vkCmdEndRenderPass = (PFN_vkCmdEndRenderPass)_LoadProcedure("vulkan-1.dll", "vkCmdEndRenderPass");
		vkQueueSubmit = (PFN_vkQueueSubmit)_LoadProcedure("vulkan-1.dll", "vkQueueSubmit");
		vkCreateFence = (PFN_vkCreateFence)_LoadProcedure("vulkan-1.dll", "vkCreateFence");
		vkDestroyFence = (PFN_vkDestroyFence)_LoadProcedure("vulkan-1.dll", "vkDestroyFence");
		vkWaitForFences = (PFN_vkWaitForFences)_LoadProcedure("vulkan-1.dll", "vkWaitForFences");
		vkGetDeviceQueue = (PFN_vkGetDeviceQueue)_LoadProcedure("vulkan-1.dll", "vkGetDeviceQueue");
		vkResetFences = (PFN_vkResetFences)_LoadProcedure("vulkan-1.dll", "vkResetFences");
		vkCreateSemaphore = (PFN_vkCreateSemaphore)_LoadProcedure("vulkan-1.dll", "vkCreateSemaphore");
		vkAcquireNextImageKHR = (PFN_vkAcquireNextImageKHR)_LoadProcedure("vulkan-1.dll", "vkAcquireNextImageKHR");
		vkResetCommandBuffer = (PFN_vkResetCommandBuffer)_LoadProcedure("vulkan-1.dll", "vkResetCommandBuffer");
		vkCmdSetScissor = (PFN_vkCmdSetScissor)_LoadProcedure("vulkan-1.dll", "vkCmdSetScissor");


		// glfw does it this way
		//vkEnumerateInstanceExtensionProperties = (PFN_vkEnumerateInstanceExtensionProperties) vkGetInstanceProcAddr( NULL, "vkEnumerateInstanceExtensionProperties" );
		//assert(vkEnumerateInstanceExtensionProperties);

		vkEnumerateInstanceExtensionProperties = (PFN_vkEnumerateInstanceExtensionProperties)_LoadProcedure("vulkan-1.dll", "vkEnumerateInstanceExtensionProperties");

#pragma endregion
		// https://docs.vulkan.org/tutorial/latest/01_Overview.html


		my->instanceExtensionNames.push_back("VK_KHR_win32_surface"); // required by vkCreateWin32SurfaceKHR()
		my->instanceExtensionNames.push_back("VK_KHR_surface"); // required by VK_KHR_surface <- vkCreateWin32SurfaceKHR()
		my->instanceExtensionNames.push_back("VK_EXT_debug_utils"); // required for debugging
		VkInstance instance = _CreateInstance(my);

		VkSurfaceKHR surface = _CreateWindowSurface(my, instance); // Khronos: The window surface needs to be created right after the instance creation, because it can actually influence the physical device selection. 

		vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		assert(vkCreateDebugUtilsMessengerEXT);
		vkCmdBindShadersEXT = (PFN_vkCmdBindShadersEXT)vkGetInstanceProcAddr(instance, "vkCmdBindShadersEXT");
		_InstallDebugCallbacks(instance);
		assert(vkCmdBindShadersEXT);

		VkPhysicalDevice physicalDevice = _ChoosePhysicalDevice(instance);

		my->deviceExtensionNames.push_back("VK_KHR_swapchain");
		my->device = _CreateLogicalDevice(my, physicalDevice, surface, &my->graphicQueIndex, &my->presentQueIndex);

		_CreatePresentationQue(physicalDevice, my->graphicQueIndex, surface);

		_CreateSwapChain(my, surface, physicalDevice);


#pragma region "shaders"

		// https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Shader_modules


		VkShaderModule fragModule;
		{
			VkShaderModuleCreateInfo moduleInfo = {};
			moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			File::file_content fragShader = File::Read("frag.spv");
			assert(fragShader.isOK);
			moduleInfo.codeSize = fragShader.count;
			moduleInfo.pCode = (uint32_t*)fragShader.bytes;
			assert(vkCreateShaderModule(my->device, &moduleInfo, NULL, &fragModule) == VK_SUCCESS);
		}

		VkShaderModule vertModule;
		{
			VkShaderModuleCreateInfo moduleInfo = {};
			moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			File::file_content vertShader = File::Read("vert.spv");
			assert(vertShader.isOK);
			moduleInfo.codeSize = vertShader.count;
			moduleInfo.pCode = (uint32_t*)vertShader.bytes;
			assert(vkCreateShaderModule(my->device, &moduleInfo, NULL, &vertModule) == VK_SUCCESS);
		}


		VkPipelineShaderStageCreateInfo shaderStageFrag = {};
		shaderStageFrag.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageFrag.module = fragModule;
		shaderStageFrag.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shaderStageFrag.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStageVert = {};
		shaderStageVert.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageVert.module = vertModule;
		shaderStageVert.stage = VK_SHADER_STAGE_VERTEX_BIT;
		shaderStageVert.pName = "main";
		// shaderStageVert.pNext = &shaderStageFrag;
		VkPipelineShaderStageCreateInfo shaderStages[] = { shaderStageVert,shaderStageFrag };

#pragma endregion

#pragma region "render pipeline"
		VkPipelineLayout layout = {};
		VkPipelineLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		assert(vkCreatePipelineLayout(my->device, &layoutInfo, NULL, &layout) == VK_SUCCESS);

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = VK_FORMAT_B8G8R8A8_SRGB;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		/*
		 DEBUG: [VUID-VkPresentInfoKHR-pImageIndices-01430] 'Validation Error: [ VUID-VkPresentInfoKHR-pImageIndices-01430 ] Object 0: handle = 0x1f9a8c35d70,
		 type = VK_OBJECT_TYPE_QUEUE; | MessageID = 0x48ad24c6 |
		 vkQueuePresentKHR(): pPresentInfo->pSwapchains[0] images passed to present must be in layout VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		 or VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR but is in VK_IMAGE_LAYOUT_UNDEFINED.
		 The Vulkan spec states: Each element of pImageIndices must be the index of a presentable image acquired from the swapchain
		 specified by the corresponding element of the pSwapchains array, and the presented image subresource must be in the
		 VK_IMAGE_LAYOUT_PRESENT_SRC_KHR or VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR layout at the time the operation is executed
		 on a VkDevice
		 (https://vulkan.lunarg.com/doc/view/1.3.268.0/windows/1.3-extensions/vkspec.html#VUID-VkPresentInfoKHR-pImageIndices-01430)'
		*/
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // also in https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Render_passes
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;

		VkAttachmentReference inputAtt = {};
		inputAtt.attachment = 0;
		inputAtt.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		//
		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		// subpass.flags = VK_SUBPASS_DESCRIPTION_RASTERIZATION_ORDER_ATTACHMENT_COLOR_ACCESS_BIT_EXT;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &inputAtt; // vkImageLayout    
		//
		renderPassInfo.subpassCount = 1; // musn't be 0
		renderPassInfo.pSubpasses = &subpass;


		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		assert(vkCreateRenderPass(my->device, &renderPassInfo, NULL, &my->renderPass) == VK_SUCCESS);


		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.renderPass = my->renderPass; //   VK_NULL_HANDLE; ... https://docs.vulkan.org/samples/latest/samples/extensions/dynamic_rendering/README.html
		pipelineInfo.layout = layout;
		VkPipelineVertexInputStateCreateInfo vertexInputState = {};
		vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		pipelineInfo.pVertexInputState = &vertexInputState;
		// <- fragment shader ... https://vulkan.lunarg.com/doc/view/1.3.268.0/windows/1.3-extensions/vkspec.html#VUID-VkGraphicsPipelineCreateInfo-pMultisampleState-09026
		VkPipelineMultisampleStateCreateInfo multisampleInfo = {};
		multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		//multisampleInfo.alphaToOneEnable = VK_TRUE;
		pipelineInfo.pMultisampleState = &multisampleInfo;
		VkPipelineInputAssemblyStateCreateInfo assInfo = {};
		assInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		assInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		assInfo.primitiveRestartEnable = VK_FALSE;
		pipelineInfo.pInputAssemblyState = &assInfo;

		VkViewport viewport = {};
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = getWidth(my->WIDTH, my->caps);
		viewport.height = getHeight(my->HEIGHT, my->caps);
		viewport.minDepth = 0;
		viewport.maxDepth = 1;
		VkPipelineViewportStateCreateInfo viewportInfo = {};
		viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportInfo.viewportCount = 1;
		viewportInfo.pViewports = &viewport;
		VkRect2D scissor = {};
		scissor.extent.width = getWidth(my->WIDTH, my->caps);
		scissor.extent.height = getHeight(my->WIDTH, my->caps);
		viewportInfo.scissorCount = 1;
		viewportInfo.pScissors = &scissor;
		pipelineInfo.pViewportState = &viewportInfo;

		VkPipelineRasterizationStateCreateInfo rasterInfo = {};
		rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
		rasterInfo.lineWidth = 1.0f;
		rasterInfo.cullMode = VK_CULL_MODE_NONE;//VK_CULL_MODE_BACK_BIT;//VK_CULL_MODE_NONE;
		rasterInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
		pipelineInfo.pRasterizationState = &rasterInfo;

		VkPipelineColorBlendAttachmentState colorBlendInfo = {};
		colorBlendInfo.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		// This is required
		VkPipelineColorBlendStateCreateInfo colorBlending = {};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendInfo;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;
		pipelineInfo.pColorBlendState = &colorBlending;

		VkPipelineCacheCreateInfo cacheInfo = {};
		cacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		VkPipelineCache pipelineCache;
		assert(vkCreatePipelineCache(my->device, &cacheInfo, NULL, &pipelineCache) == VK_SUCCESS);

		assert(pipelineInfo.pColorBlendState);
		assert(vkCreateGraphicsPipelines(my->device, pipelineCache, 1, &pipelineInfo, NULL, &my->pipeline) == VK_SUCCESS);

		//viewportInfo.scissorCount = 1;

#pragma region command buffer


// enable shader object
// add triangle
// add shader
// or add pipeline
// call shader

		uint32_t swapChainImageCount = 0;
		assert(vkGetSwapchainImagesKHR(my->device, my->swapChain, &swapChainImageCount, NULL) == VK_SUCCESS);
		VkImage* images = (VkImage*)calloc(swapChainImageCount, sizeof(VkImage));
		assert(images);
		assert(swapChainImageCount >= 1);
		assert(vkGetSwapchainImagesKHR(my->device, my->swapChain, &swapChainImageCount, images) == VK_SUCCESS);
		printf("swapchain image count = %d\n", swapChainImageCount);

		my->frameBuffers = (VkFramebuffer*)calloc(swapChainImageCount, sizeof VkFramebuffer);
		assert(my->frameBuffers);

		for (int i = 0; i < swapChainImageCount; i++) {
			VkFramebuffer frameBuffer;
			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.height = getHeight(my->HEIGHT, my->caps);
			framebufferInfo.width = getWidth(my->WIDTH, my->caps);
			framebufferInfo.renderPass = my->renderPass; // FIXME: 
			framebufferInfo.layers = 1;
			// VK_COMPONENT_SWIZZLE_IDENTITY

			VkImageView imageView;
			VkImageViewCreateInfo imageViewCreate = {};
			imageViewCreate.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewCreate.format = VK_FORMAT_B8G8R8A8_SRGB;
			imageViewCreate.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imageViewCreate.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageViewCreate.subresourceRange.levelCount = 1;
			imageViewCreate.subresourceRange.layerCount = 1;
			imageViewCreate.image = images[i];
			assert(vkCreateImageView(my->device, &imageViewCreate, NULL, &imageView) == VK_SUCCESS);

			VkImageView attachments[] = { imageView };
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments; // VkImageView
			assert(vkCreateFramebuffer(my->device, &framebufferInfo, NULL, &frameBuffer) == VK_SUCCESS);
			my->frameBuffers[i] = frameBuffer;
		}

#pragma region Prepare texture
		// https://vulkan-tutorial.com/Texture_mapping/Images
		/*
		VkImage image = {};
		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.format = VK_FORMAT_B8G8R8A8_SRGB;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		imageInfo.extent.width = WIDTH;
		imageInfo.extent.height = WIDTH;
		imageInfo.extent.depth = 1; // 32; // RGBA???
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		assert(vkCreateImage(device,&imageInfo,NULL,&image)==VK_SUCCESS);
		VkDeviceMemory deviceMemory = NULL;
		vkGetDeviceImageMemoryRequirements(device,imageRequirementsInfo,VkMemoryRequirements2)
		// vkallocaDeviceMemorya
		//VkMemoryOff
		vkBindImageMemory(device,image,deviceMemory,memoryOffset);
		*/
#pragma endregion            
		//imageInfo.extent.depth = 
		// imageViewCreate.image = image;
		// imageCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		/*
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;
		*/

		// VkShaderStageFlagBits bits;
		// VkShaderEXT shaders;
		// uint32_t stageCount;
		// vkCmdBindShadersEXT(commandBuffer,stageCount,&bits,&shaders);

		// have active render pass
		// comand buffer has recording state

		// VkDrawIndirectCommand

#pragma endregion

#pragma endregion


#pragma region Rendering        

// https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Command_buffers

		VkCommandPool commandPool;
		VkCommandPoolCreateInfo poolCreate = {};
		poolCreate.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolCreate.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolCreate.queueFamilyIndex = my->graphicQueIndex;
		assert(vkCreateCommandPool(my->device, &poolCreate, NULL, &commandPool) == VK_SUCCESS);

		VkCommandBufferAllocateInfo bufferAllocateInfo = {};
		bufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		bufferAllocateInfo.commandPool = commandPool;
		bufferAllocateInfo.commandBufferCount = 1;
		bufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		assert(vkAllocateCommandBuffers(my->device, &bufferAllocateInfo, &my->commandBuffer) == VK_SUCCESS);


		//DrawCall(0);

#pragma endregion

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		assert(vkCreateFence(my->device, &fenceInfo, NULL, &my->renderingFrameFence) == VK_SUCCESS);

		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		assert(vkCreateSemaphore(my->device, &semaphoreInfo, NULL, &my->frameReadySemaphore) == VK_SUCCESS);

	}

	void CompileShader(char* code)
	{

	}
	void BindShader()
	{

	}

	void DrawCall(MyVulkan *my,uint32_t frameBufferIndex)
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		vkBeginCommandBuffer(my->commandBuffer, &beginInfo);

		VkRenderPassBeginInfo renderBeginInfo = {};
		renderBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderBeginInfo.renderPass = my->renderPass;
		// renderBeginInfo.clearValueCount
		renderBeginInfo.framebuffer = my->frameBuffers[frameBufferIndex];
		renderBeginInfo.renderArea.extent.width = getWidth(my->WIDTH, my->caps);
		renderBeginInfo.renderArea.extent.height = getHeight(my->HEIGHT, my->caps);
		renderBeginInfo.renderArea.offset.x = 0;
		renderBeginInfo.renderArea.offset.y = 0;
		VkClearValue clearValue = {};
		clearValue.color = { .5f, 0, .5f, 1 };
		// clearValue.depthStencil
		VkClearValue clearValues[] = { clearValue };
		renderBeginInfo.clearValueCount = 1;
		renderBeginInfo.pClearValues = clearValues;
		vkCmdBeginRenderPass(my->commandBuffer, &renderBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(my->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, my->pipeline);

		if (0) // this crashes pipeline!!!
		{
			VkViewport viewport = {};
			viewport.height = getHeight(my->HEIGHT, my->caps);
			viewport.width = getWidth(my->WIDTH, my->caps);
			viewport.x = 0;
			viewport.y = 0;
			viewport.minDepth = 0;
			viewport.maxDepth = 1;
			vkCmdSetViewport(my->commandBuffer, 0, 1, &viewport);
		}

		VkRect2D scissor = {};
		scissor.extent.width = getWidth(my->WIDTH, my->caps);
		scissor.extent.height = getWidth(my->HEIGHT, my->caps);
		vkCmdSetScissor(my->commandBuffer, 0, 1, &scissor);

		vkCmdDraw(my->commandBuffer, 3, 1, 0, 0);

		vkCmdEndRenderPass(my->commandBuffer);
		vkEndCommandBuffer(my->commandBuffer);
	}

	void Display(MyVulkan *my)
	{
		//vkQueuePresentKHR(que,&presentInfo);


		// {
		//     VkSemaphoreCreateInfo semaphoreCreate = {};
		//     semaphoreCreate.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		//     VkFenceCreateInfo fenceCreate = {};
		//     fenceCreate.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		//     fenceCreate.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		//     VkFence previousFrameFence;
		//     VkFence currentFrameFence;

		//     assert(vkCreateFence(device,&fenceCreate,NULL,&previousFrameFence)==VK_SUCCESS);
		//     assert(vkCreateFence(device,&fenceCreate,NULL,&currentFrameFence)==VK_SUCCESS);

		//     assert(vkCreateSemaphore(device,&fenceCreate,NULL,&fence)==VK_SUCCESS);

		//     vkWaitForFences(device,1,&previousFrameFence,VK_TRUE,UINT64_MAX);
		//     vkResetFences(device,1,&previousFrameFence);
		// }



		// draw shit
		{
			VkQueue que = NULL;
			vkGetDeviceQueue(my->device, my->graphicQueIndex, 0, &que);
			assert(que);

			assert(vkWaitForFences(my->device, 1, &my->renderingFrameFence, VK_TRUE, UINT64_MAX) == VK_SUCCESS); // 100ms ?
			vkResetFences(my->device, 1, &my->renderingFrameFence);

			if (1)
			{
				uint32_t imageIndex = 0;
				assert(vkAcquireNextImageKHR(my->device, my->swapChain, UINT64_MAX, my->frameReadySemaphore, NULL, &imageIndex) == VK_SUCCESS);
				vkResetCommandBuffer(my->commandBuffer, 0);
				DrawCall(my,imageIndex);
			}

			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &my->commandBuffer;
			assert(vkQueueSubmit(que, 1, &submitInfo, my->renderingFrameFence) == VK_SUCCESS);

			assert(vkWaitForFences(my->device, 1, &my->renderingFrameFence, VK_TRUE, UINT64_MAX) == VK_SUCCESS); // 100ms ?
		}


		// copy drawn shit to screen
		{
			VkQueue que = NULL;
			vkGetDeviceQueue(my->device, my->presentQueIndex, 0, &que);
			assert(que);

			VkPresentInfoKHR presentInfo = {};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = &my->swapChain;
			uint32_t imageIndex = 0;
			//assert(vkAcquireNextImageKHR(device,swapChain,UINT64_MAX,frameReadySemaphore,NULL,&imageIndex)==VK_SUCCESS);
			presentInfo.pImageIndices = &imageIndex;
			vkQueuePresentKHR(que, &presentInfo);
		}
	}

	void Shutdown(MyVulkan* my)
	{
		vkDestroyFence(my->device, my->renderingFrameFence, NULL);
		CloseWindow(my->window);
	}

	VkInstance _CreateInstance(MyVulkan* my)
	{

		uint32_t allExtensionsCount = 0;
		char** allExtensionNames = NULL;
		{
			VkExtensionProperties* extensions = NULL;
			{
				assert(vkEnumerateInstanceExtensionProperties(NULL, &allExtensionsCount, NULL) == VK_SUCCESS);
				printf("[VK] %d instance extensions\n", allExtensionsCount);
				extensions = (VkExtensionProperties*)malloc(sizeof(VkExtensionProperties) * allExtensionsCount);
				assert(vkEnumerateInstanceExtensionProperties(NULL, &allExtensionsCount, extensions) == VK_SUCCESS);
				for (int i = 0; i < allExtensionsCount; i++)
				{
					printf("- '%s' @%d\n", extensions[i].extensionName, extensions[i].specVersion);
				}
			}
			assert(extensions);
			allExtensionNames = (char**)malloc(sizeof(char*) * allExtensionsCount);
			for (int i = 0; i < allExtensionsCount; i++)
			{
				allExtensionNames[i] = (char*)malloc(strlen(extensions[i].extensionName) + 1);
				strcpy(allExtensionNames[i], extensions[i].extensionName);
			}
		}

		/*
		*/
		uint32_t layersCount = 0;
		VkLayerProperties* layers;
		assert(vkEnumerateInstanceLayerProperties(&layersCount, NULL) == VK_SUCCESS);
		layers = (VkLayerProperties*)malloc(layersCount * sizeof(*layers));
		vkEnumerateInstanceLayerProperties(&layersCount, layers);
		printf("instance layer properties #%d:", layersCount);
		for (int i = 0; i < layersCount; i++)
		{
			printf("- layer '%s': '%s'\n", layers[i].layerName, layers[i].description);
		}


		VkDebugUtilsMessengerCreateInfoEXT debug1 = {};
		debug1.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debug1.messageSeverity = (
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
			//VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
			);
		debug1.messageType = (
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
			// VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT
			);
		debug1.pfnUserCallback = OnDebugUtilsMessanger;

		VkDebugReportCallbackCreateInfoEXT debug2;
		debug2.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT; // https://docs.vulkan.org/refpages/latest/refpages/source/VkStructureType.html
		debug2.flags = VK_DEBUG_REPORT_DEBUG_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT; // https://docs.vulkan.org/refpages/latest/refpages/source/VkDebugReportFlagBitsEXT.html
		debug2.pfnCallback = OnDebugReportCallbackEXT;

		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";// "What?";
		appInfo.pApplicationName = "My application name";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0; // at least 1.1 is required by maintenance extension

		// create instance
		VkResult result;
		VkInstance instance;

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO; // https://docs.vulkan.org/refpages/latest/refpages/source/VkStructureType.html
		createInfo.pNext = &debug1;
		createInfo.enabledExtensionCount = my->instanceExtensionNames.size();
		createInfo.ppEnabledExtensionNames = my->instanceExtensionNames.data();
		createInfo.enabledLayerCount = my->layerNamesCount;
		createInfo.ppEnabledLayerNames = my->layerNames;
		//createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
		createInfo.pApplicationInfo = &appInfo; // (optional)
		result = vkCreateInstance(&createInfo, VK_NULL_HANDLE, &instance);
		assert(result == VK_SUCCESS);

		return instance;
	}

	VkPhysicalDevice _ChoosePhysicalDevice(VkInstance instance)
	{
		// https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/00_Setup/03_Physical_devices_and_queue_families.html

		// choose device from physical onese - some enumeration before
		VkPhysicalDevice* devices = NULL;
		uint32_t devicesCount = 0;
		assert(vkEnumeratePhysicalDevices(instance, &devicesCount, NULL) == VK_SUCCESS);
		assert(devicesCount >= 1);
		devices = (VkPhysicalDevice*)calloc(devicesCount, sizeof(devices[0]));
		assert(vkEnumeratePhysicalDevices(instance, &devicesCount, devices) == VK_SUCCESS);
		assert(devices);


		VkPhysicalDeviceProperties deviceProperties = {};
		vkGetPhysicalDeviceProperties(devices[0], &deviceProperties);
		assert(deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);

		return devices[0];
	}

	VkDevice _CreateLogicalDevice(MyVulkan *my, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, int* graphicQueIndex, int* presentQueIndex)
	{
		// https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/00_Setup/03_Physical_devices_and_queue_families.html#_queue_families

		uint32_t queCount = 0;
		VkQueueFamilyProperties* queProps = NULL;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queCount, NULL);
		assert(queCount > 0);
		queProps = (VkQueueFamilyProperties*)calloc(queCount, sizeof(queProps[0]));
		assert(queProps);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queCount, queProps);
		//
		for (int i = 0; i < queCount; i++)
		{
			VkBool32 supported;
			assert(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supported) == VK_SUCCESS);
			if ((*presentQueIndex < 0) && supported == VK_TRUE) // we support only the same que atm
			{
				*presentQueIndex = i;
			}

			if (queProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				*graphicQueIndex = i;
				// break;
			}
		}

		assert((*graphicQueIndex) >= 0);
		assert((*presentQueIndex) >= 0);
		assert(*presentQueIndex == *graphicQueIndex); // We support only this case atm

		printf("PRESENT QUE = %d GRAPHIC QUE = %d\n", *presentQueIndex, *graphicQueIndex);

		VkDeviceQueueCreateInfo queCreateInfo = {};
		float quePriorities[] = { 1.0 };
		queCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queCreateInfo.queueCount = 1;
		queCreateInfo.queueFamilyIndex = *graphicQueIndex;
		queCreateInfo.pQueuePriorities = quePriorities;
		VkDevice device;

		uint32_t allDeviceExtensionsCount = 0;
		char** allDeviceExtensionNames = NULL;
		{
			VkExtensionProperties* extensions = NULL;
			{
				assert(vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &allDeviceExtensionsCount, NULL) == VK_SUCCESS);
				printf("[VK] %d physical device extensions:\n", allDeviceExtensionsCount);
				extensions = (VkExtensionProperties*)malloc(sizeof(VkExtensionProperties) * allDeviceExtensionsCount);
				assert(vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &allDeviceExtensionsCount, extensions) == VK_SUCCESS);
				for (int i = 0; i < allDeviceExtensionsCount; i++)
				{
					printf("- '%s' @%d\n", extensions[i].extensionName, extensions[i].specVersion);
				}
			}
			assert(extensions);
			allDeviceExtensionNames = (char**)malloc(sizeof(char*) * allDeviceExtensionsCount);
			for (int i = 0; i < allDeviceExtensionsCount; i++)
			{
				printf("- extension %d [%s]\n", i, extensions[i].extensionName);
				allDeviceExtensionNames[i] = (char*)malloc(strlen(extensions[i].extensionName) + 1);
				strcpy(allDeviceExtensionNames[i], extensions[i].extensionName);
			}
		}

		VkPhysicalDeviceFeatures deviceFeatures = {};
		vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
		deviceCreateInfo.ppEnabledExtensionNames = my->deviceExtensionNames.data();
		deviceCreateInfo.enabledExtensionCount = my->deviceExtensionNames.size();
		deviceCreateInfo.queueCreateInfoCount = 1;
		deviceCreateInfo.pQueueCreateInfos = &queCreateInfo;
		deviceCreateInfo.enabledLayerCount = my->layerNamesCount;
		deviceCreateInfo.ppEnabledLayerNames = my->layerNames;
		VkResult result = vkCreateDevice(physicalDevice, &deviceCreateInfo, NULL, &device);
		assert(result == VK_SUCCESS);

		return device;
	}

}