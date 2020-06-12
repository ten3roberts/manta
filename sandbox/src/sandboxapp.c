#include "manta.h"

static Window* window = NULL;

int swapchain_resize = 0;

int application_start(int argc, char** argv)
{
	Timer timer = timer_start(CT_WALL_TICKS);

	time_init();

	settings_load();

	window = window_create("sandbox", settings_get_resolution().x, settings_get_resolution().y, settings_get_window_style(), 1);
	window_set_icon(window, "./assets/textures/ridge64.png", "./assets/textures/ridge1024.png");

	input_init(window);
	graphics_init(window, GLOBAL_LAYOUT_DEFAULT);
	renderer_init();

	LOG_S("Initialization took %f ms", timer_stop(&timer) * 1000);

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

	Camera* camera = camera_create_perspective("main", (Transform){(vec3){0, 0, 10}}, window_get_aspect(window), 1.5, 0.1, 100);

	Entity* entity1 = entity_create("entity1", "grid", "cube", (Transform){(vec3){0, 0, -10}, quat_identity, vec3_one}, rigidbody_stationary);

	(void)entity_create("entity2", "concrete", "cube", (Transform){(vec3){4, 0, -10}, quat_identity, vec3_one}, (Rigidbody){.velocity = (vec3){-5, 0, 0}});

	Entity* entity3 = entity_create("suzanne", "concrete", "multiple:Suzanne", (Transform){(vec3){0, 0, 3}, quat_identity, vec3_one}, rigidbody_stationary);

	for (int i = 0; i < 10000; i++)
	{
		Entity* e = entity_create("multiple", "concrete", "cube", (Transform){vec3_add(vec3_random_sphere_even(10, 200), (vec3){0, -0, 0}), quat_identity, (vec3){0.5f, 0.5f, 0.5f}},
								  (Rigidbody){.velocity = vec3_random_sphere(0, 1)});

		// Assign a random color
		entity_set_color(e, vec4_hsv(rand() / (float)RAND_MAX * 2 * M_PI, rand() / (float)RAND_MAX, rand() / (float)RAND_MAX));
	}

	while (!window_get_close(window))
	{
		// Poll window events
		window_update(window);
		renderer_begin();


		entity_get_transform(entity1)->rotation = quat_euler((vec3){0, time_elapsed(), 0});
		//entity_get_transform(entity2)->rotation =
		//	quat_mul(quat_euler((vec3){0, 0, time_elapsed() * 4}), quat_euler((vec3){0, time_elapsed() * 0.75, 0}));
		entity_get_transform(entity3)->rotation = quat_euler((vec3){0, time_elapsed() * 0.5, 0});
		entity_set_color(entity3, (vec4){0, 0, 1, 1});
		vec3 cam_move = vec3_zero;

		float speed = 15;
		if (input_key(KEY_LEFT_SHIFT))
			speed *= 3;

		if (input_key(KEY_W))
		{
			cam_move.z = -speed;
		}
		if (input_key(KEY_S))
		{
			cam_move.z = speed;
		}
		if (input_key(KEY_A))
		{
			cam_move.x = -speed;
		}
		if (input_key(KEY_D))
		{
			cam_move.x = speed;
		}
		// Spin camera

		static vec3 cam_rot = (vec3){0};
#define SENSITIVITY 0.001f;
		cam_rot.x -= input_mouse_rel().y * SENSITIVITY;
		cam_rot.y += input_mouse_rel().x * SENSITIVITY;

		camera_get_transform(camera)->rotation = quat_euler(cam_rot);
		entity_get_transform(entity3)->rotation = quat_euler(cam_rot);

		transform_translate(camera_get_transform(camera), vec3_scale(cam_move, time_delta()));
		transform_translate(entity_get_transform(entity3), vec3_scale(cam_move, time_delta()));
		scene_update(scene);
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
			LOG("Framerate %10d %10f", time_framecount(), time_framerate());
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
		renderer_hint_resize();
	}

	static enum CursorMode mode = CURSORMODE_NORMAL;
	if (event.type == EVENT_KEY && event.idata[0] == KEY_ESCAPE && event.idata[1] == 1)
	{
		if (mode == CURSORMODE_LOCKED)
			mode = CURSORMODE_NORMAL;
		else if (mode == CURSORMODE_NORMAL)
			mode = CURSORMODE_LOCKED;

		window_set_cursor_mode(window, mode);
	}

	/*if (event.type == EVENT_KEY)
		LOG("Key pressed  : %d, %c", event.idata[0], event.idata[0]);*/
	input_send_event(&event);
}

void* application_get_window()
{
	return window;
}
