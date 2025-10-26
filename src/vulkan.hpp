#include "assert.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

namespace VK
{
    HWND window = NULL;

    //#define PFN_vkVoidFunction void*

    //#define VK_DEFINE_HANDLE(object) typedef struct object##_T* object;
    //VK_DEFINE_HANDLE(VkInstance)
    
    //#define vk_fun(ret, name,args) typedef ret (*name##_t) args; name##_t name;
    //#include "vulkan_functions.template"

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

        // https://docs.vulkan.org/tutorial/latest/01_Overview.html

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
        VkResult result = VK_TIMEOUT;
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

        // https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/00_Setup/03_Physical_devices_and_queue_families.html

        // choose device from physical onese - some enumeration before
        VkPhysicalDevice * devices = NULL;
        uint32_t devicesCount = 0; 
        assert(vkEnumeratePhysicalDevices(instance,&devicesCount,NULL) == VK_SUCCESS);
        assert(devicesCount >= 1);
        devices = (VkPhysicalDevice*) calloc(devicesCount, sizeof(devices[0]));
        assert(vkEnumeratePhysicalDevices(instance,&devicesCount,devices) == VK_SUCCESS);
        assert(devices);

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
        vkCreateWin32SurfaceKHR(instance, &createInfo32, NULL, &surface);

        VkPhysicalDeviceProperties deviceProperties = {};
        VkPhysicalDeviceFeatures deviceFeatures = {};
        vkGetPhysicalDeviceProperties(devices[0], &deviceProperties);
        vkGetPhysicalDeviceFeatures(devices[0], &deviceFeatures);
        assert(deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
        // assert(deviceFeatures.deviceType === VK_PHYSICAL_DEVICE_FEEATURE_);
        //printf("")
        //deviceProperties.vendorID

        // FIXME: create logical device - what I will be using ???

        // https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/00_Setup/03_Physical_devices_and_queue_families.html#_queue_families
        uint32_t queCount = 0;
        VkQueueFamilyProperties * queProps = NULL;
        vkGetPhysicalDeviceQueueFamilyProperties(devices[0], &queCount, NULL);
        assert(queCount > 0);
        queProps = (VkQueueFamilyProperties*) calloc(queCount, sizeof(queProps[0]));
        assert(queProps);
        vkGetPhysicalDeviceQueueFamilyProperties(devices[0], &queCount, queProps);
        
        int graphicQueIndex = -1;
        for(int i=0;i<queCount;i++)
        {
            if(queProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                graphicQueIndex = i;
                break;
            }
        }

        
        VkDeviceQueueCreateInfo queCreateInfo = {};
        queCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queCreateInfo.queueCount = 1;
        queCreateInfo.queueFamilyIndex = graphicQueIndex;
        VkDevice device;


        uint32_t deviceExtensionsCount = 0;
        char ** deviceExtensionNames = NULL;
        {
            VkExtensionProperties* extensions = NULL;
            {
                assert(vkEnumerateDeviceExtensionProperties(devices[0],NULL,&deviceExtensionsCount,NULL)==VK_SUCCESS);
                printf("[VK] %d physical device extensions:\n", deviceExtensionsCount);
                extensions = (VkExtensionProperties*) malloc(sizeof(VkExtensionProperties) * deviceExtensionsCount);
                assert(vkEnumerateDeviceExtensionProperties(devices[0],NULL,&deviceExtensionsCount,extensions)==VK_SUCCESS);
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


        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
        deviceCreateInfo.ppEnabledExtensionNames = deviceExtensionNames;
        deviceCreateInfo.enabledExtensionCount = deviceExtensionsCount;
        deviceCreateInfo.queueCreateInfoCount = 1;
        deviceCreateInfo.pQueueCreateInfos = &queCreateInfo;        
        result = vkCreateDevice(devices[0],&deviceCreateInfo,NULL,&device);
        assert( result == VK_SUCCESS );

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
}