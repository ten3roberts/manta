#ifndef INPUT_H
#define INPUT_H
#include "event.h"
#include "keycodes.h"
#include "math/vec.h"
#include "window.h"

// Mouse buttons counts as keys

void input_init(Window* window);

void input_send_event(Event* event);

// Needs to be called every frame before events are polled
void input_update();

// Returns true if the key is being pressed down
int input_key(int keycode);

// Returns true if the key was pressed this frame
int input_key_pressed(int keycode);

// Returns true if the key was released this frame
int input_key_released(int keycode);

// Returns the mouse position as a vec2
// Measures the cursor position as pixels from the top left corner
vec2 input_mouse_pos();

// Returns the mouse position as a vec2
// Measures the cursor position as a factor of the windows dimensions
// E.g; top left corner (0,0), bottom right corner (1,1)
vec2 input_mouse_pos_norm();

// Returns the length scrolled since the application started
// Can be floating point but is often an integer representing the amount of 'notches' scrolled
// The y component represents the normal vertical scroll wheel or touchpad
// The x component represents the horizontal scroll wheel or touchpad
vec2 input_get_scroll();

// Returns the length scrolled horizontal and vertical since the last frame
// Can be floating point but is often an integer representing the amount of 'notches' scrolled
// The y component represents the normal vertical scroll wheel or touchpad
// The x component represents the horizontal scroll wheel or touchpad
vec2 input_get_rel_scroll();
#endif