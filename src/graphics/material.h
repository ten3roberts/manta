#include <stdint.h>
#include <vulkan/vulkan.h>

typedef struct Material Material;

Material* material_create(const char* file);

// Bind the material's pipeline
// Binds a material's descriptors for the specified frame
// If frame is -1, the current frame to render will be used (result of renderer_get_frame)
void material_bind(Material* mat, VkCommandBuffer command_buffer, uint32_t frame);

// Destroys a single material
void material_destroy(Material* mat);

// Destroys all materials
void material_destroy_all();