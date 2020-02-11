typedef struct Texture Texture;
#include "vulkan/vulkan.h"

Texture* texture_create(const char* file);

void texture_destroy(Texture* tex);

void texture_bind(Texture* tex, VkCommandBuffer command_buffer, int i);

void* texture_get_image_view(Texture* tex);

void* texture_get_sampler(Texture* tex);