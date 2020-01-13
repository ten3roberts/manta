#pragma once
#include "event.h"


int application_start();

// The starting point from where all events should propagate
void application_send_event(Event event);