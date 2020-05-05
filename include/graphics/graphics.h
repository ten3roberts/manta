#ifndef GRAPHICS_H
#define GRAPHICS_H
#include "window.h"
#include "graphics/camera.h"
#include "graphics/shadertypes.h"


#define GLOBAL_LAYOUT_DEFAULT NULL
// Initialized the graphics api
// The global_layout is a struct specifying the global uniform buffers and textures the shaders use
// Note: The first binding is reserved to scene data and should be atleast sizeof(struct SceneData) large
// If global_layout is NULL, the default layout will be used
int graphics_init(Window* window, struct LayoutInfo* global_layout);

// Returns the surface window
Window* graphics_get_window();

// Updates a global uniform buffer at the specified binding with the supplied data
// The first binding is usually scene data
int graphics_update_buffer(uint32_t binding, void* data, uint32_t offset, uint32_t size);

// This will update the global scene data to the shader
// Assumes the first binding is scene data
// This is just a wrapper for 'graphics_update_buffer' with scene data supplied
// Updates all cameras position for the current frame
int graphics_update_scene_data();

// Terminates and frees the graphics api
// Should be called before the window or glfw is terminated
void graphics_terminate();

// Swapchain when using vulkan
int swapchain_create();
int swapchain_recreate();
int swapchain_destroy();
#endif