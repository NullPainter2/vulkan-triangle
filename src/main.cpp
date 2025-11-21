#include <windows.h>
#include <stdio.h>
#include <iostream>
//#include <string>
//#include <vector>
// #include <assert.h>

#include "vulkan.hpp"


int foo() {
    return 3;
}

void DrawTriangle(VK::MyVulkan *v)
{
    // char * vertexShader = "";
    char * pixelShader = "out vec3 color; main(){ color = vec3(1.,0,1.); }";
    VK::CompileShader(pixelShader);
    VK::BindShader();
    //VK::DrawCall();
    VK::Display(v);

    foo();

}

void WaitForExit()
{
    Sleep(2000);
}

int main( int argc, char **argv )
{
    VK::MyVulkan v;


    VK::Init(&v);
    DrawTriangle(&v);
    WaitForExit();
    VK::Shutdown(&v);
    return 0;
}