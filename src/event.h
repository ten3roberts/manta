#pragma once
typedef enum
{
	EVENT_NONE = 0,
	EVENT_KEY,
	EVENT_MOUSE_SCROLLED,
	EVENT_MOUSE_MOVED,
    EVENT_WINDOW_SIZE,
    EVENT_WINDOW_FOCUS,
	EVENT_WINDOW_CLOSE

} EVENT_TYPE;

// Defines an event structure
// Event data can be represented as either an integer or float
typedef struct
{
	EVENT_TYPE type;
	// A union representing the event data in either integer or float form
	union {
		int idata[2];
        float fdata[2];
	};
	int handled;
} Event;
