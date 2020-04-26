#ifndef GRAPHICS_H
#define GRAPHICS_H

// Initialized the graphics api
// The implementation location of this function is determined by which api is used
int graphics_init();

// Terminates and frees the graphics api
// Should be called before the window or glfw is terminated
// The implementation location of this function is determined by which api is used
void graphics_terminate();

// Swapchain when using vulkan
int swapchain_create();
int swapchain_recreate();
int swapchain_destroy();
#endif