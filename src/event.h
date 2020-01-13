#pragma once
typedef enum
{
	EVENT_NONE = 0,
	EVENT_KEY_RELEASED,
	EVENT_KEY_PRESSED,
	EVENT_MOUSE_SCROLLED,
	EVENT_MOUSE_MOVED,
    EVENT_WINDOW_SIZE,
    EVENT_WINDOW_FOCUS,
	EVENT_WINDOW_CLOSE

} EVENT_TYPE;

// Defines a normal event structure
typedef struct
{
	EVENT_TYPE type;
	//union {
		int data[2];
        //float fdata[2];
	//};
} Event;
