#pragma once
#include "event.h"

// Defines the application
// Handles events and propogates them throughout the program
// Handles the main loop and updates the application

// Starts the application
// Initalizes time
// Initializes input
// Creates a window
// Initializes vulkan
// Enter main loop
// terminates vulkan
// destroys the window
int application_start();

// The starting point from where all events should propagate
void application_send_event(Event event);

// Returns the current window of the application
// Returns as a void pointer as window.h is not included to to deps
// Can safely be cast to Window unless null
void* application_get_window();