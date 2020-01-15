#pragma once
#include "event.h"
#include "keycodes.h"


// Mouse buttons counts as keys

void input_init();

void input_send_event(Event* event);

// Needs to be called before events are polled
void input_update();

// Returns true if the key is being pressed down
int input_key(int keycode);

// Returns true if the key was pressed this frame
int input_key_pressed(int keycode);

// Returns true if the key was released this frame
int input_key_released(int keycode);