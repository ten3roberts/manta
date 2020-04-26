#ifndef RENDERER_H
#define RENDERER_H

#include <stdint.h>

// Initializes rendering for the next frame
// Acquires the next image in the swapchain
// Render queue can be rerecorded
// Uniform buffers and other shader resources should now be updated
void renderer_begin();

// Handles synchronization
// Submits the render command buffer to the GPU
// Presents the correct framebuffer
void renderer_draw();

// Retrieves the index of the current image to render to
uint32_t renderer_get_frameindex();
#endif