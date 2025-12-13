/*
 * apt install vulkan-tools
 */
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define VK_USE_PLATFORM_XLIB_KHR
#include <vulkan/vulkan.h>

#include "vulkan.hpp"

void DrawTriangle(VK::MyVulkan *v)
{
    // char * vertexShader = "";
    char * pixelShader = "out vec3 color; main(){ color = vec3(1.,0,1.); }";
    VK::CompileShader(pixelShader);
    VK::BindShader();
    //VK::DrawCall();
    VK::Display(v);

}

void WaitForExit()
{
    // Sleep(2000);
    getchar();
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


#if 0
int main()
{
    int width = 800;
    int height = 600;

    Display * display = XOpenDisplay( NULL );
    assert(display);
    int screen = DefaultScreen( display );
    Window parent = RootWindow( display, screen );
    Window window = XCreateSimpleWindow(display, parent, 0, 0, width, height, 0, 0, 0);
    assert(window);
    XSelectInput(
			display,
			window,
			KeyPressMask | // keypress
			ExposureMask | // resize event
			ResizeRedirectMask |
			0
	);
	XMapWindow( display, window ); // ShowWindow, otherwise just enters loop with one event

	XEvent event = {};
	//
	auto dll = dlopen("libvulkan.so", RTLD_NOW);
	assert(dll);

	VK::MyVulkan


	//
	while(true)
	{
	    if( XEventsQueued( display, QueuedAfterReading ))
		{
		    XNextEvent( display, &event );
			if( event.type == Expose )
			{
			    // draw
			}
		}

	}
/*
screenBuffer = XCreateImage(
    display,
    DefaultVisual( display, screen ), // visual,
    DefaultDepth( display, screen ), // depth,
    ZPixmap, // format,
    0, // offset,
    nullptr, // data,
    width,
    height,
    32, // bitmap_pad,
    0 // bytes_per_line
);
 */
    printf("xxx");
    return 0;
}
#endif
