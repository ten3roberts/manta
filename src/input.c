#include "input.h"
#include "keycodes.h"
#include <string.h>
#include "log.h"
int keys[CR_KEY_LAST + 1];
int prev_keys[CR_KEY_LAST + 1];

void input_init()
{
	memset(keys, 0, sizeof(keys));
	memset(prev_keys, 0, sizeof(prev_keys));
}

void input_send_event(Event* event)
{

	if (event->type != EVENT_KEY)
		return;

	// If event is not handled update the key
	if (event->handled == 0)
		keys[event->idata[0]] = event->idata[1];

	// If event is handled, only release the key without triggering input_key_release
	else if (event->idata[1] == 0)
	{
		keys[event->idata[0]] = 0;
		prev_keys[event->idata[0]] = 0;
	}
	event->handled = 1;
}

void input_update()
{
	memcpy(prev_keys, keys, sizeof keys);
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
