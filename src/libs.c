#include "log.h"

// A C file for building and configuring header only libraries
#define JSON_IMPLEMENTATION
#define JSON_MESSAGE(m) LOG_E(m)
#include "libjson.h"