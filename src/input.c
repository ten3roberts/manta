#include "input.h"
#include "keycodes.h"
#include <string.h>
#include "log.h"
#include "math/vec.h"
#include "application.h"

Window* _window;

int keys[KEY_LAST + 1];
int prev_keys[KEY_LAST + 1];

vec2 mouse_pos;

vec2 scroll;
vec2 rel_scroll;

void input_init(Window* window)
{
	memset(keys, 0, sizeof(keys));
	memset(prev_keys, 0, sizeof(prev_keys));

	mouse_pos = (vec2){0, 0};
	scroll = (vec2){0, 0};
	rel_scroll = (vec2){0, 0};

	_window = window;
}

void input_send_event(Event* event)
{
	if (event->type == EVENT_KEY)
	{

		// If event is not handled update the key
		if (event->handled == 0)
			keys[event->idata[0]] = event->idata[1];

		// If event is handled, only release the key without triggering input_key_release
		else if (event->idata[1] == 0)
		{
			keys[event->idata[0]] = 0;
			prev_keys[event->idata[0]] = 0;
		}
	}

	// Update mouse position
	else if (event->type == EVENT_MOUSE_MOVED)
	{
		mouse_pos.x = event->fdata[0];
		mouse_pos.y = event->fdata[1];
	}
	else if (event->type == EVENT_MOUSE_SCROLLED)
	{
		scroll.x += event->fdata[0];
		scroll.y += event->fdata[1];
		rel_scroll.x += event->fdata[0];
		rel_scroll.y += event->fdata[1];
	}
	event->handled = 1;
}

void input_update()
{
	memcpy(prev_keys, keys, sizeof keys);
	rel_scroll = (vec2){0, 0};
}

int input_key(int keycode)
{
	return keys[keycode];
}

int input_key_pressed(int keycode)
{
	return keys[keycode] && !prev_keys[keycode];
}

int input_key_released(int keycode)
{
	return !keys[keycode] && prev_keys[keycode];
}

vec2 input_mouse_pos()
{
	return mouse_pos;
}

vec2 input_mouse_pos_norm()
{
	return (vec2){mouse_pos.x / window_get_width(_window), mouse_pos.y / window_get_height(_window)};
}

vec2 input_get_scroll()
{
	return scroll;
}

vec2 input_get_rel_scroll()
{
	return rel_scroll;
}