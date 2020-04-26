
#ifndef APPLICATION_H
#define APPLICATION_H
#include "event.h"

// Includes the declarations of the user application functions

// Defines the application
// Handles events and propogates them throughout the program
// Handles the main loop and updates the application

// A user application defined function that is called to start the application
// All initialization happend here
// The user handles the game loop, be it this or a separate function call
extern int application_start(int argc, char** argv);

// The starting point from where all events should propagate
extern void application_send_event(Event event);

// Returns the current window of the application
// Returns as a void pointer as window.h is not included to to deps
// Can safely be cast to Window unless null
extern void* application_get_window();
#endif