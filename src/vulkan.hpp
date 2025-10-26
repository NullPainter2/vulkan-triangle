#include "assert.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include "File.hpp"

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

namespace VK
{
    VkDevice _CreateLogicalDevice(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, int * graphicQueIndex, int * presentQueIndex);
    VkPhysicalDevice _ChoosePhysicalDevice(VkInstance instance);
    VkInstance _CreateInstance();

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
        window = CreateWindowExA(0,windowClass.lpszClassName,"Vulkan Triangle!",WS_TILEDWINDOW | WS_VISIBLE,0,0,800,600,NULL,NULL,NULL,NULL);
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
        VkResult result = vkCreateSwapchainKHR(device,&swapInfo,NULL,&swapChain);
        assert(result==VK_SUCCESS);
    }

    void Init()
    {
        // get loader https://docs.vulkan.org/guide/latest/loader.html
        vkCreateInstance = (PFN_vkCreateInstance) _LoadProcedure( "vulkan-1.dll", "vkCreateInstance" );
        vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr) _LoadProcedure( "vulkan-1.dll", "vkGetInstanceProcAddr" );
        vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR) _LoadProcedure( "vulkan-1.dll", "vkCreateWin32SurfaceKHR" );
        vkEnumerateInstanceExtensionProperties = (PFN_vkEnumerateInstanceExtensionProperties) _LoadProcedure( "vulkan-1.dll", "vkEnumerateInstanceExtensionProperties" );
        vkEnumerateInstanceLayerProperties = (PFN_vkEnumerateInstanceLayerProperties) _LoadProcedure( "vulkan-1.dll", "vkEnumerateInstanceLayerProperties" );
        vkEnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices) _LoadProcedure( "vulkan-1.dll", "vkEnumeratePhysicalDevices" );
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

        // https://docs.vulkan.org/tutorial/latest/01_Overview.html


        int graphicQueIndex = -1;
        int presentQueIndex = -1;

        VkInstance instance = _CreateInstance();

        VkSurfaceKHR surface = _CreateWindowSurface(instance); // Khronos: The window surface needs to be created right after the instance creation, because it can actually influence the physical device selection. 

        vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance,"vkCreateDebugUtilsMessengerEXT");
        _InstallDebugCallbacks(instance);

        VkPhysicalDevice physicalDevice = _ChoosePhysicalDevice( instance );

        VkDevice device = _CreateLogicalDevice(physicalDevice, surface, &graphicQueIndex, &presentQueIndex);

        _CreatePresentationQue(physicalDevice,graphicQueIndex,surface);

        _CreateSwapChain(device,surface,physicalDevice);

        
        // https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Shader_modules

        VkShaderModuleCreateInfo moduleInfo = {};
        moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        
        {
            File::file_content fragShader = File::Read("frag.spv");
            assert(fragShader.isOK);
            moduleInfo.codeSize = fragShader.count;
            moduleInfo.pCode = (uint32_t*) fragShader.bytes;        
            VkShaderModule module;
            assert(vkCreateShaderModule(device,&moduleInfo,NULL,&module)==VK_SUCCESS);
        }

        {
            File::file_content fragShader = File::Read("vert.spv");
            moduleInfo.codeSize = fragShader.count;
            moduleInfo.pCode = (uint32_t*) fragShader.bytes;        
            VkShaderModule module;
            assert(vkCreateShaderModule(device,&moduleInfo,NULL,&module)==VK_SUCCESS);
        }
    }
    void CompileShader(char *code)
    {

    }
    void BindShader()
    {

    }
    void DrawCall()
    {

    }
    void Display()
    {

    }

    void Shutdown()
    {
        CloseWindow(window);
    }

    VkInstance _CreateInstance()
    {

        uint32_t extensionsCount = 0;
        char ** extensionNames = NULL;
        {
            VkExtensionProperties* extensions = NULL;
            {
                assert(vkEnumerateInstanceExtensionProperties(NULL,&extensionsCount,NULL)==VK_SUCCESS);
                printf("[VK] %d properties\n", extensionsCount);
                extensions = (VkExtensionProperties*) malloc(sizeof(VkExtensionProperties) * extensionsCount);
                assert(vkEnumerateInstanceExtensionProperties(NULL,&extensionsCount,extensions)==VK_SUCCESS);
                for(int i=0;i<extensionsCount;i++)
                {
                    printf("- '%s' @%d\n",extensions[i].extensionName, extensions[i].specVersion);
                }
            }
            assert(extensions);
            extensionNames = (char**) malloc(sizeof(char*) * extensionsCount);
            for(int i=0;i<extensionsCount;i++)
            {
                extensionNames[i] = (char*) malloc(strlen(extensions[i].extensionName)+1);
                strcpy(extensionNames[i], extensions[i].extensionName);
            }
        }
        
        /* throws exception !!!

        uint32_t layersCount = 0;
        VkLayerProperties *layers;
        assert(vkEnumerateInstanceLayerProperties(&layersCount,NULL) == VK_SUCCESS);
        layers = (VkLayerProperties*) malloc(layersCount * sizeof(*layers));
        vkEnumerateInstanceLayerProperties(&layersCount,layers);
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
        appInfo.apiVersion = VK_API_VERSION_1_0;

        // create instance
        VkResult result;
        VkInstance instance;
 
        VkInstanceCreateInfo createInfo = {};        
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO; // https://docs.vulkan.org/refpages/latest/refpages/source/VkStructureType.html
        createInfo.pNext = &debug1;
        createInfo.enabledExtensionCount = extensionsCount;
        createInfo.ppEnabledExtensionNames = extensionNames;
        char * layers[] = {"VK_LAYER_KHRONOS_validation"};
        createInfo.enabledLayerCount = 1;
        createInfo.ppEnabledLayerNames = layers;
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
        queCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queCreateInfo.queueCount = 1;
        queCreateInfo.queueFamilyIndex = *graphicQueIndex;
        VkDevice device;

        uint32_t deviceExtensionsCount = 0;
        char ** deviceExtensionNames = NULL;
        {
            VkExtensionProperties* extensions = NULL;
            {
                assert(vkEnumerateDeviceExtensionProperties(physicalDevice,NULL,&deviceExtensionsCount,NULL)==VK_SUCCESS);
                printf("[VK] %d physical device extensions:\n", deviceExtensionsCount);
                extensions = (VkExtensionProperties*) malloc(sizeof(VkExtensionProperties) * deviceExtensionsCount);
                assert(vkEnumerateDeviceExtensionProperties(physicalDevice,NULL,&deviceExtensionsCount,extensions)==VK_SUCCESS);
                for(int i=0;i<deviceExtensionsCount;i++)
                {
                    printf("- '%s' @%d\n",extensions[i].extensionName, extensions[i].specVersion);
                }
            }
            assert(extensions);
            deviceExtensionNames = (char**) malloc(sizeof(char*) * deviceExtensionsCount);
            for(int i=0;i<deviceExtensionsCount;i++)
            {
                printf("- extension [%s]\n", extensions[i].extensionName);
                deviceExtensionNames[i] = (char*) malloc(strlen(extensions[i].extensionName)+1);
                strcpy(deviceExtensionNames[i], extensions[i].extensionName);
            }
        }

        VkPhysicalDeviceFeatures deviceFeatures = {};
        vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
        deviceCreateInfo.ppEnabledExtensionNames = deviceExtensionNames;
        deviceCreateInfo.enabledExtensionCount = deviceExtensionsCount;
        deviceCreateInfo.queueCreateInfoCount = 1;
        deviceCreateInfo.pQueueCreateInfos = &queCreateInfo;        
        VkResult result = vkCreateDevice(physicalDevice,&deviceCreateInfo,NULL,&device);
        assert( result == VK_SUCCESS );

        return device;
    }

}