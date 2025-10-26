#include <windows.h>
#include <stdio.h>
#include <iostream>
//#include <string>
//#include <vector>
// #include <assert.h>

#include "vulkan.hpp"

void DrawTriangle()
{
    // char * vertexShader = "";
    char * pixelShader = "out vec3 color; main(){ color = vec3(1.,0,1.); }";
    VK::CompileShader(pixelShader);
    VK::BindShader();
    VK::DrawCall();
    VK::Display();
}

void WaitForExit()
{
    Sleep(2000);
}

int main( int argc, char **argv )
{
    VK::Init();
    DrawTriangle();
    WaitForExit();
    VK::Shutdown();
    return 0;
}