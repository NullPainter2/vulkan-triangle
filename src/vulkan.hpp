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
    VkDevice _CreateLogicalDevice(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, int * graphicQueIndex, int * presentQueIndex);
    VkPhysicalDevice _ChoosePhysicalDevice(VkInstance instance);
    VkInstance _CreateInstance();

    char * layerNames[] = {"VK_LAYER_KHRONOS_validation"};
    int layerNamesCount = 1;

    // auto extensionNames = std::vector<char*>();
    auto instanceExtensionNames = std::vector<char*>();
    auto deviceExtensionNames = std::vector<char*>();
    //char * extensionNames[] =  {""};
    //int extensionNamesCount = 0;

    int WIDTH = 800;
    int HEIGHT = 600;

    VkPipeline pipeline = {};

    HWND window = NULL;

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

    void * _LoadProcedure(char *dllName, char *procName)
    {
        auto dll = LoadLibraryA(dllName);
        assert(dll);
        void * result = GetProcAddress(dll,procName);
        assert(result);
        return result;
    }

    VkBool32 OnDebugReportCallbackEXT(
        VkDebugReportFlagsEXT                       flags,
        VkDebugReportObjectTypeEXT                  objectType,
        uint64_t                                    object,
        size_t                                      location,
        int32_t                                     messageCode,
        const char*                                 pLayerPrefix,
        const char*                                 pMessage,
        void*                                       pUserData
    )
    {
        printf("DEBUG: [%d] '%s'\n", flags, pMessage ? pMessage : "???");
        return VK_FALSE; // required
    }

    VkBool32 OnDebugUtilsMessanger(
        VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
        const VkDebugUtilsMessengerCallbackDataEXT*      pCallbackData,
        void*                                            pUserData)
    {
        assert(pCallbackData);
        printf("DEBUG: [%s] '%s'\n", pCallbackData->pMessageIdName, pCallbackData->pMessage);
        return VK_FALSE;
    }

    bool quitFromMessageHandler = false;

    LRESULT CALLBACK
    _OnWindowMessage( HWND window, UINT message_id, WPARAM wparam, LPARAM lparam )
    {
        if( message_id == WM_SIZE )
        {
            //GPU::win32_resizeScreenBuffer( window );

            return 0;
        }
        else if( message_id == WM_CLOSE )
        {
            quitFromMessageHandler = true;

            return 0;
        }
        else
        {
            return DefWindowProcA( window, message_id, wparam, lparam );
        }
    }

    VkSurfaceKHR _CreateWindowSurface(VkInstance instance)
    {
        // https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/01_Presentation/00_Window_surface.html

        WNDCLASSA windowClass = {};
        windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        windowClass.lpszClassName = "my-window-class";
        windowClass.lpfnWndProc = _OnWindowMessage;
        windowClass.hInstance = GetModuleHandle( nullptr );
        assert(RegisterClassA(&windowClass));
        window = CreateWindowExA(0,windowClass.lpszClassName,"Vulkan Triangle!",WS_TILEDWINDOW | WS_VISIBLE,0,0,WIDTH,HEIGHT,NULL,NULL,NULL,NULL);
        assert(window);
        VkWin32SurfaceCreateInfoKHR createInfo32 = {};
        createInfo32.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo32.hwnd = window;
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
        assert(vkCreateDebugUtilsMessengerEXT(instance,&debug1,NULL,&debugHandle) == VK_SUCCESS);
    }

    void _CreatePresentationQue(VkPhysicalDevice physicalDevice, uint32_t graphicFamilyIndex, VkSurfaceKHR surface)
    {
    }

    void _CreateSwapChain(VkDevice device, VkSurfaceKHR surface, VkPhysicalDevice physicalDevice)
    {
        VkSwapchainKHR swapChain;
        VkSwapchainCreateInfoKHR swapInfo = {};
        swapInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapInfo.surface = surface;
        // swapInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        
        uint32_t formatsCount = 0;
        VkSurfaceFormatKHR * formats = NULL;
        assert(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice,surface,&formatsCount,NULL)==VK_SUCCESS);
        formats = (VkSurfaceFormatKHR*) calloc(formatsCount,sizeof(formats[0]));
        assert(formats);
        assert(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice,surface,&formatsCount,formats)==VK_SUCCESS);
        printf("[INFO] format count %d\n",formatsCount);
        int bestFormatIndex = -1;
        for(int i=0;i<formatsCount;i++)
        {
            if(formats[i].colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR && formats[i].format == VK_FORMAT_B8G8R8A8_SRGB) //  VK_FORMAT_R8G8B8_SRGB ?
            {
                // TODO: print those
                bestFormatIndex = i;
                break;
            }
        }
        assert(bestFormatIndex >= 0);

        VkPresentModeKHR presentMode;
        VkPresentModeKHR * presents;
        uint32_t presentsCount;
        assert(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice,surface,&presentsCount,NULL)==VK_SUCCESS);
        printf("[INFO] present modes count %d\n",presentsCount);
        assert(presentsCount > 0);
        presents = (VkPresentModeKHR*) calloc(presentsCount, sizeof(presents[0]));
        assert(presents);
        assert(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice,surface,&presentsCount,presents)==VK_SUCCESS);

        VkSurfaceCapabilitiesKHR caps = {};
        assert(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface,&caps) == VK_SUCCESS);
        swapInfo.minImageCount = caps.minImageCount;
        swapInfo.imageFormat = formats[bestFormatIndex].format; // VK_FORMAT_B8G8R8A8_SRGB
        swapInfo.imageColorSpace = formats[bestFormatIndex].colorSpace;
        swapInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapInfo.preTransform = caps.currentTransform;
        swapInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapInfo.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR; // TODO: make sure it is in `presents` array
        swapInfo.imageArrayLayers = 1; // not stereoscopic 3d
        VkExtent2D extent = {};
        extent.width = caps.minImageExtent.width;//WIDTH;
        extent.height = caps.minImageExtent.height;//HEIGHT;
        swapInfo.imageExtent = extent;
        VkResult result = vkCreateSwapchainKHR(device,&swapInfo,NULL,&swapChain);
        assert(result==VK_SUCCESS);
    }

    void Init()
    {
        #pragma region "load vulkan libraries from dll"
            
            // get loader https://docs.vulkan.org/guide/latest/loader.html
            vkCreateInstance = (PFN_vkCreateInstance) _LoadProcedure( "vulkan-1.dll", "vkCreateInstance" );
            vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr) _LoadProcedure( "vulkan-1.dll", "vkGetInstanceProcAddr" );
            vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR) _LoadProcedure( "vulkan-1.dll", "vkCreateWin32SurfaceKHR" );
            vkEnumerateInstanceLayerProperties = (PFN_vkEnumerateInstanceLayerProperties) _LoadProcedure( "vulkan-1.dll", "vkEnumerateInstanceLayerProperties" );
            vkEnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices) _LoadProcedure( "vulkan-1.dll", "vkEnumeratePhysicalDevices" );
            // device
            vkGetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties) _LoadProcedure( "vulkan-1.dll", "vkGetPhysicalDeviceProperties" );
            vkGetPhysicalDeviceQueueFamilyProperties = (PFN_vkGetPhysicalDeviceQueueFamilyProperties) _LoadProcedure( "vulkan-1.dll", "vkGetPhysicalDeviceQueueFamilyProperties" );
            vkGetPhysicalDeviceFeatures = (PFN_vkGetPhysicalDeviceFeatures) _LoadProcedure( "vulkan-1.dll", "vkGetPhysicalDeviceFeatures" );
            vkCreateDevice = (PFN_vkCreateDevice) _LoadProcedure( "vulkan-1.dll", "vkCreateDevice" );
            vkEnumerateDeviceExtensionProperties = (PFN_vkEnumerateDeviceExtensionProperties) _LoadProcedure( "vulkan-1.dll", "vkEnumerateDeviceExtensionProperties" );
            vkGetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR) _LoadProcedure( "vulkan-1.dll", "vkGetPhysicalDeviceSurfaceSupportKHR" );
            vkCreateSwapchainKHR = (PFN_vkCreateSwapchainKHR) _LoadProcedure( "vulkan-1.dll", "vkCreateSwapchainKHR" );
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR) _LoadProcedure( "vulkan-1.dll", "vkGetPhysicalDeviceSurfaceCapabilitiesKHR" );
            vkGetPhysicalDeviceSurfaceFormatsKHR = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR) _LoadProcedure( "vulkan-1.dll", "vkGetPhysicalDeviceSurfaceFormatsKHR" );
            vkGetPhysicalDeviceSurfacePresentModesKHR = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR) _LoadProcedure( "vulkan-1.dll", "vkGetPhysicalDeviceSurfacePresentModesKHR" );
            vkCreateShaderModule = (PFN_vkCreateShaderModule) _LoadProcedure( "vulkan-1.dll", "vkCreateShaderModule" );
            vkCreateShaderModule = (PFN_vkCreateShaderModule) _LoadProcedure("vulkan-1.dll", "vkCreateShaderModule");
            vkCreatePipelineCache = (PFN_vkCreatePipelineCache) _LoadProcedure("vulkan-1.dll", "vkCreatePipelineCache");
            vkCreateGraphicsPipelines = (PFN_vkCreateGraphicsPipelines) _LoadProcedure("vulkan-1.dll", "vkCreateGraphicsPipelines");
            vkBeginCommandBuffer = (PFN_vkBeginCommandBuffer) _LoadProcedure("vulkan-1.dll", "vkBeginCommandBuffer");
            vkCmdBindPipeline = (PFN_vkCmdBindPipeline) _LoadProcedure("vulkan-1.dll", "vkCmdBindPipeline");
            vkCmdDraw = (PFN_vkCmdDraw) _LoadProcedure("vulkan-1.dll", "vkCmdDraw");
            vkQueuePresentKHR = (PFN_vkQueuePresentKHR) _LoadProcedure("vulkan-1.dll", "vkQueuePresentKHR");        
            vkEndCommandBuffer = (PFN_vkEndCommandBuffer) _LoadProcedure("vulkan-1.dll", "vkEndCommandBuffer");
            vkCreatePipelineLayout = (PFN_vkCreatePipelineLayout) _LoadProcedure("vulkan-1.dll", "vkCreatePipelineLayout");
            vkCreateRenderPass = (PFN_vkCreateRenderPass) _LoadProcedure("vulkan-1.dll", "vkCreateRenderPass");

            // glfw does it this way
            //vkEnumerateInstanceExtensionProperties = (PFN_vkEnumerateInstanceExtensionProperties) vkGetInstanceProcAddr( NULL, "vkEnumerateInstanceExtensionProperties" );
            //assert(vkEnumerateInstanceExtensionProperties);

            vkEnumerateInstanceExtensionProperties = (PFN_vkEnumerateInstanceExtensionProperties) _LoadProcedure( "vulkan-1.dll", "vkEnumerateInstanceExtensionProperties" );

        #pragma endregion
        // https://docs.vulkan.org/tutorial/latest/01_Overview.html


        int graphicQueIndex = -1;
        int presentQueIndex = -1;

        instanceExtensionNames.push_back("VK_KHR_win32_surface"); // required by vkCreateWin32SurfaceKHR()
        instanceExtensionNames.push_back("VK_KHR_surface"); // required by VK_KHR_surface <- vkCreateWin32SurfaceKHR()
        instanceExtensionNames.push_back("VK_EXT_debug_utils"); // required for debugging
        VkInstance instance = _CreateInstance();

        VkSurfaceKHR surface = _CreateWindowSurface(instance); // Khronos: The window surface needs to be created right after the instance creation, because it can actually influence the physical device selection. 

        vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance,"vkCreateDebugUtilsMessengerEXT");
        assert(vkCreateDebugUtilsMessengerEXT);
        vkCmdBindShadersEXT = (PFN_vkCmdBindShadersEXT) vkGetInstanceProcAddr(instance,"vkCmdBindShadersEXT");
        _InstallDebugCallbacks(instance);
        assert(vkCmdBindShadersEXT);

        VkPhysicalDevice physicalDevice = _ChoosePhysicalDevice( instance );

        deviceExtensionNames.push_back("VK_KHR_swapchain");
        VkDevice device = _CreateLogicalDevice(physicalDevice, surface, &graphicQueIndex, &presentQueIndex);

        _CreatePresentationQue(physicalDevice,graphicQueIndex,surface);

        _CreateSwapChain(device,surface,physicalDevice);

        
        #pragma region "shaders"

            // https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Shader_modules

            VkShaderModuleCreateInfo moduleInfo = {};
            moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            
            VkShaderModule fragModule;
            {
                File::file_content fragShader = File::Read("frag.spv");
                assert(fragShader.isOK);
                moduleInfo.codeSize = fragShader.count;
                moduleInfo.pCode = (uint32_t*) fragShader.bytes;        
                assert(vkCreateShaderModule(device,&moduleInfo,NULL,&fragModule)==VK_SUCCESS);
            }

            VkShaderModule vertModule;
            {
                File::file_content vertShader = File::Read("vert.spv");
                moduleInfo.codeSize = vertShader.count;
                moduleInfo.pCode = (uint32_t*) vertShader.bytes;        
                assert(vkCreateShaderModule(device,&moduleInfo,NULL,&vertModule)==VK_SUCCESS);
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
            shaderStageVert.pNext = &shaderStageFrag;

        #pragma endregion

        #pragma region "render pipeline"
            VkPipelineLayout layout = {};
            VkPipelineLayoutCreateInfo layoutInfo = {};
            layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            assert(vkCreatePipelineLayout(device,&layoutInfo,NULL,&layout)==VK_SUCCESS);

            VkRenderPass renderPass = {};
            VkRenderPassCreateInfo renderPassInfo = {};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            VkAttachmentReference inputAtt = {};
            inputAtt.attachment = 1;
            inputAtt.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            VkSubpassDescription subpass = {};    
            //subpass.flags = VK_SUBPASS_DESCRIPTION_RASTERIZATION_ORDER_ATTACHMENT_COLOR_ACCESS_BIT_EXT;
            //subpass.inputAttachmentCount = 0;
            // subpass.pInputAttachments = &inputAtt; // vkImageLayout    
            renderPassInfo.subpassCount = 1; // musn't be 0
            renderPassInfo.pSubpasses = &subpass;
            assert(vkCreateRenderPass(device,&renderPassInfo,NULL,&renderPass)==VK_SUCCESS);


            VkGraphicsPipelineCreateInfo pipelineInfo = {};
            pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipelineInfo.stageCount = 1;
            pipelineInfo.pStages = &shaderStageVert;
            pipelineInfo.renderPass =  renderPass; //   VK_NULL_HANDLE; ... https://docs.vulkan.org/samples/latest/samples/extensions/dynamic_rendering/README.html
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
            // assInfo.primitiveRestartEnable = VK_FALSE
            pipelineInfo.pInputAssemblyState  = &assInfo;
            VkPipelineCacheCreateInfo cacheInfo = {};
            cacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
            VkPipelineCache pipelineCache;
            assert(vkCreatePipelineCache(device,&cacheInfo,NULL,&pipelineCache)==VK_SUCCESS);
            assert(vkCreateGraphicsPipelines(device,pipelineCache,1,&pipelineInfo,NULL,&pipeline)==VK_SUCCESS);
        #pragma endregion
    }
    void CompileShader(char *code)
    {

    }
    void BindShader()
    {

    }
    void DrawCall()
    {
        VkCommandBuffer commandBuffer = {};
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;        
        vkBeginCommandBuffer(commandBuffer,&beginInfo);

        // enable shader object
        // add triangle
        // add shader
        // or add pipeline
        // call shader
        VkPipelineBindPoint bindPoint = {};
        vkCmdBindPipeline(commandBuffer,bindPoint,pipeline);

        VkShaderStageFlagBits bits;
        VkShaderEXT shaders;
        uint32_t stageCount;
        vkCmdBindShadersEXT(commandBuffer,stageCount,&bits,&shaders);

        vkEndCommandBuffer(commandBuffer);
        vkCmdDraw(commandBuffer,3,0,0,0);
        // VkDrawIndirectCommand
    }
    void Display()
    {
        VkPresentInfoKHR presentInfo;
        VkQueue que;
        vkQueuePresentKHR(que,&presentInfo);
    }

    void Shutdown()
    {
        CloseWindow(window);
    }

    VkInstance _CreateInstance()
    {

        uint32_t allExtensionsCount = 0;
        char ** allExtensionNames = NULL;
        {
            VkExtensionProperties* extensions = NULL;
            {
                assert(vkEnumerateInstanceExtensionProperties(NULL,&allExtensionsCount,NULL)==VK_SUCCESS);
                printf("[VK] %d instance extensions\n", allExtensionsCount);
                extensions = (VkExtensionProperties*) malloc(sizeof(VkExtensionProperties) * allExtensionsCount);
                assert(vkEnumerateInstanceExtensionProperties(NULL,&allExtensionsCount,extensions)==VK_SUCCESS);
                for(int i=0;i<allExtensionsCount;i++)
                {
                    printf("- '%s' @%d\n",extensions[i].extensionName, extensions[i].specVersion);
                }
            }
            assert(extensions);
            allExtensionNames = (char**) malloc(sizeof(char*) * allExtensionsCount);
            for(int i=0;i<allExtensionsCount;i++)
            {
                allExtensionNames[i] = (char*) malloc(strlen(extensions[i].extensionName)+1);
                strcpy(allExtensionNames[i], extensions[i].extensionName);
            }
        }
        
        /*
        uint32_t layersCount = 0;
        VkLayerProperties *layers;
        assert(vkEnumerateInstanceLayerProperties(&layersCount,NULL) == VK_SUCCESS);
        layers = (VkLayerProperties*) malloc(layersCount * sizeof(*layers));
        vkEnumerateInstanceLayerProperties(&layersCount,layers);
        printf("instance layer properties #%d:",layersCount);
        for(int i=0;i<layersCount;i++)
        {
            printf("- layer '%s': '%s'\n",layers[i].layerName, layers[i].description);
        }
            */


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
        debug2.flags = VK_DEBUG_REPORT_DEBUG_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT  | VK_DEBUG_REPORT_DEBUG_BIT_EXT; // https://docs.vulkan.org/refpages/latest/refpages/source/VkDebugReportFlagBitsEXT.html
        debug2.pfnCallback = OnDebugReportCallbackEXT; 

        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.applicationVersion = VK_MAKE_VERSION(1,0,0);
        appInfo.pEngineName = "No Engine";// "What?";
        appInfo.pApplicationName = "My application name";
        appInfo.engineVersion = VK_MAKE_VERSION(1,0,0);
        appInfo.apiVersion = VK_API_VERSION_1_0; // at least 1.1 is required by maintenance extension

        // create instance
        VkResult result;
        VkInstance instance;
 
        VkInstanceCreateInfo createInfo = {};        
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO; // https://docs.vulkan.org/refpages/latest/refpages/source/VkStructureType.html
        createInfo.pNext = &debug1;
        createInfo.enabledExtensionCount = instanceExtensionNames.size();
        createInfo.ppEnabledExtensionNames = instanceExtensionNames.data();
        createInfo.enabledLayerCount = layerNamesCount;
        createInfo.ppEnabledLayerNames = layerNames;
        //createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        createInfo.pApplicationInfo = &appInfo; // (optional)
        result = vkCreateInstance(&createInfo,VK_NULL_HANDLE,&instance);
        assert(result == VK_SUCCESS);        

        return instance;
    }

    VkPhysicalDevice _ChoosePhysicalDevice(VkInstance instance)
    {
        // https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/00_Setup/03_Physical_devices_and_queue_families.html

        // choose device from physical onese - some enumeration before
        VkPhysicalDevice * devices = NULL;
        uint32_t devicesCount = 0; 
        assert(vkEnumeratePhysicalDevices(instance,&devicesCount,NULL) == VK_SUCCESS);
        assert(devicesCount >= 1);
        devices = (VkPhysicalDevice*) calloc(devicesCount, sizeof(devices[0]));
        assert(vkEnumeratePhysicalDevices(instance,&devicesCount,devices) == VK_SUCCESS);
        assert(devices);


        VkPhysicalDeviceProperties deviceProperties = {};
        vkGetPhysicalDeviceProperties(devices[0], &deviceProperties);
        assert(deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);

        return devices[0];
    }
    
    VkDevice _CreateLogicalDevice(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, int * graphicQueIndex, int * presentQueIndex)
    {
        // https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/00_Setup/03_Physical_devices_and_queue_families.html#_queue_families

        uint32_t queCount = 0;
        VkQueueFamilyProperties * queProps = NULL;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queCount, NULL);
        assert(queCount > 0);
        queProps = (VkQueueFamilyProperties*) calloc(queCount, sizeof(queProps[0]));
        assert(queProps);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queCount, queProps);
        //
        for(int i=0;i<queCount;i++)
        {
            VkBool32 supported;
            if(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice,i,surface,&supported))
            {
                *presentQueIndex = i;
            }
            if(queProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                *graphicQueIndex = i;
                // break;
            }
        }

        assert(graphicQueIndex >= 0);
        assert(presentQueIndex >= 0);
        
        VkDeviceQueueCreateInfo queCreateInfo = {};
        float quePriorities[] = {1.0};
        queCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queCreateInfo.queueCount = 1;
        queCreateInfo.queueFamilyIndex = *graphicQueIndex;
        queCreateInfo.pQueuePriorities = quePriorities;
        VkDevice device;

        uint32_t allDeviceExtensionsCount = 0;
        char ** allDeviceExtensionNames = NULL;
        {
            VkExtensionProperties* extensions = NULL;
            {
                assert(vkEnumerateDeviceExtensionProperties(physicalDevice,NULL,&allDeviceExtensionsCount,NULL)==VK_SUCCESS);
                printf("[VK] %d physical device extensions:\n", allDeviceExtensionsCount);
                extensions = (VkExtensionProperties*) malloc(sizeof(VkExtensionProperties) * allDeviceExtensionsCount);
                assert(vkEnumerateDeviceExtensionProperties(physicalDevice,NULL,&allDeviceExtensionsCount,extensions)==VK_SUCCESS);
                for(int i=0;i<allDeviceExtensionsCount;i++)
                {
                    printf("- '%s' @%d\n",extensions[i].extensionName, extensions[i].specVersion);
                }
            }
            assert(extensions);
            allDeviceExtensionNames = (char**) malloc(sizeof(char*) * allDeviceExtensionsCount);
            for(int i=0;i<allDeviceExtensionsCount;i++)
            {
                printf("- extension %d [%s]\n", i, extensions[i].extensionName);
                allDeviceExtensionNames[i] = (char*) malloc(strlen(extensions[i].extensionName)+1);
                strcpy(allDeviceExtensionNames[i], extensions[i].extensionName);
            }
        }

        VkPhysicalDeviceFeatures deviceFeatures = {};
        vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
        deviceCreateInfo.ppEnabledExtensionNames = deviceExtensionNames.data();
        deviceCreateInfo.enabledExtensionCount = deviceExtensionNames.size();
        deviceCreateInfo.queueCreateInfoCount = 1;
        deviceCreateInfo.pQueueCreateInfos = &queCreateInfo;      
        deviceCreateInfo.enabledLayerCount = layerNamesCount;
        deviceCreateInfo.ppEnabledLayerNames = layerNames;
        VkResult result = vkCreateDevice(physicalDevice,&deviceCreateInfo,NULL,&device);
        assert( result == VK_SUCCESS );

        return device;
    }

}