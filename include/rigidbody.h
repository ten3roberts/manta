#ifndef RIGIDBODY_H
#define RIGIDBODY_H
#include "math/vec.h"
#include "transform.h"

typedef struct Rigidbody
{
	vec3 velocity;
	/* data */
} Rigidbody;

const static Rigidbody rigidbody_stationary = {.velocity = (vec3){0, 0, 0}};

void rigidbody_update(Rigidbody* rigidbody, Transform* transform);
#endif