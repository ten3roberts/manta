#include "rigidbody.h"
#include "cr_time.h"

void rigidbody_update(Rigidbody* rigidbody, Transform* transform)
{
	transform->position = vec3_add(transform->position, vec3_scale(rigidbody->velocity, time_delta()));
}
