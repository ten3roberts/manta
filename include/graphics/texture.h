#ifndef TEXTURE_H
#define TEXTURE_H

// TODO: remove vulkan header dependency
typedef struct Texture Texture;
#include "vulkan/vulkan.h"

//Texture* texture_create(const char* file);

// Attempts to find a texture by filename and if it doesn't exist it gets loaded
Texture* texture_get(const char* name);

void texture_destroy(Texture* tex);

// Destroys all loaded textures
void texture_destroy_all();

void* texture_get_image_view(Texture* tex);

void* texture_get_sampler(Texture* tex);
#endif
