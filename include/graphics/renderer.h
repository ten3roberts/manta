#ifndef RENDERER_H
#define RENDERER_H

#include <stdint.h>
#include "entity.h"

// Initializes the renderer
void renderer_init();

// Initializes rendering for the next frame
// Acquires the next image in the swapchain
// Render queue can be rerecorded
// Uniform buffers and other shader resources should now be updated
void renderer_begin();

// Handles synchronization
// Submits the render command buffer to the GPU
// Presents the correct framebuffer
void renderer_submit();

// This function shall be called after a resize event
// Does not explicitely resize the window, but recreates the swapchain and pipelines
// Will not immidiately resize until resize events stop
// This is to avoid resizing every frame when user drag-resizes window
void renderer_resize();

// Will frag to the renderer that the command buffers need to be rebuilt
// The command buffers will be redrawn on submit
// Later versions will use secondary cmd buffer to allow for partial reconstruction
void renderer_flag_rebuild();

// Retrieves the index of the current image to render to
uint32_t renderer_get_frameindex();
#endif