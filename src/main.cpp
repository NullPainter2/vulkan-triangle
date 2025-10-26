#include <windows.h>
#include <stdio.h>
#include <iostream>
//#include <string>
//#include <vector>
// #include <assert.h>

#include "vulkan.hpp"


void DrawTriangle()
{

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
    return 0;
}