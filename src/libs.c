#include "log.h"
#define MP_MESSAGE LOG
#ifdef DEBUG
#define MP_CHECK_FULL
#else
#define MP_DISABLE
#endif
#define MP_IMPLEMENTATION
#include "magpie.h"

#ifdef DEBUG
// Catch hashtable allocations
#endif

// A C file for building and configuring header only libraries
#define LIBJSON_IMPLEMENTATION
#define LIBJSON_MESSAGE(m) LOG_E(m)
#include "libjson.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define HASHTABLE_IMPLEMENTATION
#include "hashtable.h"
#define HANDLETABLE_IMPLEMENTATION
#define handletable_create(keyfunc, hashfunc, compfunc) mp_bind(handletable_create(keyfunc, hashfunc, compfunc))
#include "handletable.h"
#define MEMPOOL_MAGPIE
#define MEMPOOL_IMPLEMENTATION
#define MEMPOOL_MESSAGE(m) LOG_E(m)
#include "mempool.h"
