#pragma once

/* Initialized vulkan for renderering */
int vulkan_init();

/* Terminates and frees the vulkan resources 
Should be called before the window or glfw is terminated*/
void vulkan_terminate();