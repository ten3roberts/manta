#include "log.h"
#define MP_MESSAGE LOG
#if DEBUG
#define MP_CHECK_FULL
#else
#define MP_DISABLE
#endif
#define MP_IMPLEMENTATION
#include "magpie.h"

// A C file for building and configuring header only libraries
#define JSON_IMPLEMENTATION
#define JSON_MESSAGE(m) LOG_E(m)
#include "libjson.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
