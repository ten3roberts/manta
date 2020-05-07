#ifndef RIGIDBODY_H
#define RIGIDBODY_H
#include "math/vec.h"
#include "transform.h"

typedef struct Rigidbody
{
	vec3 velocity;
	/* data */
} Rigidbody;


void rigidbody_update(Rigidbody* rigidbody, Transform* transform);
#endif