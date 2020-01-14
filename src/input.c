#include "input.h"
#include "keycodes.h"
#include <string.h>
#include "log.h"
int keys[CR_KEY_LAST+1];
int prev_keys[CR_KEY_LAST+1];

void input_init()
{
	memset(keys, 0, sizeof(keys));
	memset(prev_keys, 0, sizeof(prev_keys));
}

void input_send_event(Event * event)
{

	if (event->type == EVENT_KEY_PRESSED && !event->handled)
	{
		keys[event->idata[0]] = 1;
	}
	else if (event->type == EVENT_KEY_RELEASED)
	{
		keys[event->idata[0]] = 0;

		if (event->handled)
			prev_keys[event->idata[0]] = 0;
	}
	event->handled = 1;
}

void input_update()
{
	memcpy(prev_keys, keys, sizeof keys);
}

int input_getkey(int keycode)
{
	return keys[keycode];
}

int input_getkey_pressed(int keycode)
{
	return keys[keycode] && !prev_keys[keycode];
}

int input_getkey_released(int keycode)
{
	return !keys[keycode] && prev_keys[keycode];
}
