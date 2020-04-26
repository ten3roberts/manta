#ifndef EVENT_H
#define EVENT_H
typedef enum
{
	// Dummy or invalid event
	EVENT_NONE = 0,
	// A key or mouse button event
	// { keycode, 0 = released : 1 = pressed }
	EVENT_KEY,
	// Scroll wheel event
	// { xScroll, yScroll }
	EVENT_MOUSE_SCROLLED,
	// Mouse moved event
	// { new x, new y }
	EVENT_MOUSE_MOVED,

	// Window resize event
	// { new width, new height }
	EVENT_WINDOW_RESIZE,
	// Window focus or lost focus event
	// { 0 = unfocused : 1 = focused, NULL }
	EVENT_WINDOW_FOCUS,
	// Window X, or closed event
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
#endif