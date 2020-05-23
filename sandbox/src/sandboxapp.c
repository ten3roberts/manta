#include "manta.h"

static Window* window = NULL;

int swapchain_resize = 0;

int application_start(int argc, char** argv)
{
	Timer timer = timer_start(CT_WALL_TICKS);

	time_init();

	settings_load();

	window = window_create("sandbox", settings_get_resolution().x, settings_get_resolution().y, settings_get_window_style(), 0);
	window_set_icon(window, "./assets/textures/ridge64.png", "./assets/textures/ridge1024.png");

	input_init(window);
	graphics_init(window, GLOBAL_LAYOUT_DEFAULT);
	renderer_init();

	LOG_S("Initialization took %f ms", timer_stop(&timer) * 1000);
	LOG_ASSERT(false, "Window was not NULL");
	if (log_get_count(LOG_SEVERITY_ERROR) > 0)
	{
		LOG("Errors encountered during initialization, exiting");
		SLEEP(5);
		exit(-1);
	}

	timer_reset(&timer);
	time_init();

	Scene* scene = scene_create("main");
	model_load_collada("./assets/models/cube.dae");
	model_load_collada("./assets/models/multiple.dae");
	material_load("./assets/materials/concrete.json");
	material_load("./assets/materials/grid.json");

	Camera* camera = camera_create_perspective("main", (Transform){(vec3){0, -10, 10}}, window_get_aspect(window), 1.5, 0.1, 100);

	Entity* entity1 = entity_create("entity1", "grid", "cube", (Transform){(vec3){0, 0, -10}, quat_identity, vec3_one}, rigidbody_stationary);
	Entity* entity2 = entity_create("entity2", "concrete", "cube", (Transform){(vec3){4, 0, -10}, quat_identity, vec3_one}, (Rigidbody){.velocity = (vec3){-5, 0, 0}});
	Entity* entity3 = entity_create("entity2", "concrete", "multiple:Suzanne", (Transform){(vec3){-2, 2, -10}, quat_identity, vec3_one}, rigidbody_stationary);

	/*for (int i = 0; i < 1000; i++)
	{
		entity_create("entity_mult", "grid", "Cube", (Transform){.position = vec3_random_sphere_even(10, 100)});
	}*/
	for (int i = 0; i < 10000; i++)
	{
		entity_create("multiple", "grid", "cube", (Transform){vec3_add(vec3_random_sphere_even(10, 200), (vec3){0, -0, 0}), quat_identity, (vec3){0.5f, 0.5f, 0.5f}},
					  (Rigidbody){.velocity = vec3_random_sphere(0, 5)});
	}

	while (!window_get_close(window))
	{
		// Poll window events
		window_update(window);
		renderer_begin();

		scene_update(scene);

		entity_get_transform(entity1)->rotation = quat_euler((vec3){0, time_elapsed(), 0});
		//entity_get_transform(entity2)->rotation =
		//	quat_mul(quat_euler((vec3){0, 0, time_elapsed() * 4}), quat_euler((vec3){0, time_elapsed() * 0.75, 0}));
		entity_get_transform(entity3)->rotation = quat_euler((vec3){0, time_elapsed() * 0.5, 0});
		vec3 cam_move = vec3_zero;
		if (input_key(KEY_W))
		{
			cam_move.z = -10;
		}
		if (input_key(KEY_S))
		{
			cam_move.z = 10;
		}
		if (input_key(KEY_A))
		{
			cam_move.x = -10;
		}
		if (input_key(KEY_D))
		{
			cam_move.x = 10;
		}
		// Spin camera
		camera_get_transform(camera)->position = vec3_add(camera_get_transform(camera)->position, vec3_scale(cam_move, time_delta()));
		//entity_get_transform(entity2)->position = vec3_add(entity_get_transform(entity2)->position, vec3_scale(cam_move, time_delta()));
		graphics_update_scene_data();
		input_update();

		time_update();

		//const SphereCollider* bound1 = entity_get_boundingsphere(entity1);
		//const SphereCollider* bound2 = entity_get_boundingsphere(entity2);

		/*if (spherecollider_intersect(bound1, bound2))
		{
			LOG("Colliding");
		}
		else
		{
			LOG("Not Colliding");
		}*/

		renderer_submit(scene);

		if (timer_duration(&timer) > 2.0f)
		{
			timer_reset(&timer);
			LOG("Framerate %d %f", time_framecount(), time_framerate());
		}
	}
	scene_destroy_entities(scene);

	camera_destroy(camera);
	scene_destroy(scene);
	renderer_terminate();
	graphics_terminate();
	LOG_S("Terminating");
	window_destroy(window);
	settings_save();

	return 0;
}

void application_send_event(Event event)
{
	// Recreate swapchain on resize
	if (event.type == EVENT_WINDOW_RESIZE && event.idata[0] != 0 && event.idata[1] != 0)
	{
		LOG("%d %d", event.idata[0], event.idata[1]);
		renderer_resize();
	}
	/*if (event.type == EVENT_KEY)
		LOG("Key pressed  : %d, %c", event.idata[0], event.idata[0]);*/
	input_send_event(&event);
}

void* application_get_window()
{
	return window;
}
