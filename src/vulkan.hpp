#include "assert.h"

namespace VK
{
    #define PFN_vkVoidFunction void*

    #define VK_DEFINE_HANDLE(object) typedef struct object##_T* object;
    VK_DEFINE_HANDLE(VkInstance)
    // #define VkInstance int
    
    #define vk_fun(name,ret,args) typedef ret (*name##_t) args; name##_t name;
    #include "vulkan_functions.template"

    void * _LoadProcedure(char *dllName, char *procName)
    {
        auto dll = GetModuleHandleA(dllName);
        assert(dll);
        void * result = GetProcAddress(dll,procName);
        assert(result);
        return result;
    }

    void Init()
    {
        vkGetInstanceProcAddr = (vkGetInstanceProcAddr_t) _LoadProcedure( "vulkan-1.dll", "vkGetInstanceProcAddr" );        
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
}