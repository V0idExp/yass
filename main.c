#include "error.h"
#include "font.h"
#include "game.h"
#include "matlib.h"
#include "memory.h"
#include "renderer.h"
#include "script.h"
#include "shader.h"
#include "sprite.h"
#include "strutils.h"
#include "text.h"
#include <GL/glew.h>
#include <SDL.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/*** RESOURCES ***/
static struct Sprite *spr_player = NULL;
static struct Sprite *spr_enemy_01 = NULL;
static struct Sprite *spr_asteroid_01 = NULL;
static struct Sprite *spr_projectile_01 = NULL;
static struct Font *font_dbg = NULL;
static struct Text *fps_text = NULL;
static struct Text *render_time_text = NULL;

static int
load_resources(void)
{
	// load sprites
	const char *sprite_files[] = {
		"data/art/playerShip1_blue.png",
		"data/art/Enemies/enemyBlack2.png",
		"data/art/Meteors/meteorGrey_small2.png",
		"data/art/Lasers/laserBlue07.png",
		NULL
	};
	struct Sprite **sprites[] = {
		&spr_player,
		&spr_enemy_01,
		&spr_asteroid_01,
		&spr_projectile_01
	};
	for (int i = 0; sprite_files[i] != NULL; i++) {
		if (!(*sprites[i] = sprite_from_file(sprite_files[i]))) {
			fprintf(stderr, "failed to load sprite `%s`\n", sprite_files[i]);
			return 0;
		}
		printf("loaded sprite `%s`\n", sprite_files[i]);
	}

	// load fonts
	const char *font_files[] = {
		"data/fonts/courier.ttf",
		NULL
	};
	int font_sizes[] = {
		16,
	};
	struct Font **fonts[] = {
		&font_dbg,
	};
	for (int i = 0; font_files[i] != NULL; i++) {
		if (!(*fonts[i] = font_from_file(font_files[i], font_sizes[i]))) {
			fprintf(stderr, "failed to load font `%s`\n", font_files[i]);
			return 0;
		}
		printf("loaded font `%s`\n", font_files[i]);
	}

	// create text renderables
	fps_text = text_new(font_dbg);
	render_time_text = text_new(font_dbg);
	if (!fps_text || !render_time_text) {
		return 0;
	}

	return 1;
}

static void
cleanup_resources(void)
{
	text_destroy(render_time_text);
	text_destroy(fps_text);
	font_destroy(font_dbg);
	sprite_destroy(spr_player);
	sprite_destroy(spr_asteroid_01);
	sprite_destroy(spr_projectile_01);
}

static void
render_world(struct RenderList *rndr_list, struct World *world)
{
	render_list_add_sprite(
		rndr_list,
		spr_player,
		world->player.x,
		world->player.y,
		0.0f
	);

	struct ListNode *ast_node = world->asteroid_list->head;
	while (ast_node) {
		struct Asteroid *ast = ast_node->data;
		render_list_add_sprite(
			rndr_list,
			spr_asteroid_01,
			ast->x,
			ast->y,
			ast->rot
		);
		ast_node = ast_node->next;
	}

	struct ListNode *prj_node = world->projectile_list->head;
	while (prj_node) {
		struct Projectile *prj = prj_node->data;
		if (prj->ttl > 0) {
			render_list_add_sprite(
				rndr_list,
				spr_projectile_01,
				prj->x,
				prj->y,
				0
			);
		}
		prj_node = prj_node->next;
	}

	struct ListNode *enemy_node = world->enemy_list->head;
	while (enemy_node) {
		struct Enemy *enemy = enemy_node->data;
		render_list_add_sprite(
			rndr_list,
			spr_enemy_01,
			enemy->x,
			enemy->y,
			0
		);
		enemy_node = enemy_node->next;
	}
}

static void
render_ui(struct RenderList *rndr_list)
{
	// render FPS indicator
	render_list_add_text(
		rndr_list,
		fps_text,
		-SCREEN_WIDTH / 2,
		-SCREEN_HEIGHT / 2
	);

	// render render time indicator
	render_list_add_text(
		rndr_list,
		render_time_text,
		-SCREEN_WIDTH / 2,
		-SCREEN_HEIGHT / 2 + 20
	);
}

static int
handle_key(const SDL_Event *key_evt, struct World *world)
{
	// handle player actions
	int act = 0;
	switch (key_evt->key.keysym.sym) {
	case SDLK_a:
	case SDLK_LEFT:
		act = ACTION_MOVE_LEFT;
		break;
	case SDLK_d:
	case SDLK_RIGHT:
		act = ACTION_MOVE_RIGHT;
		break;
	case SDLK_SPACE:
		act = ACTION_SHOOT;
		break;
	}

	if (key_evt->type == SDL_KEYUP) {
		world->player.actions &= ~act;
	} else {
		world->player.actions |= act;
	}
	return 1;
}

int
main(int argc, char *argv[])
{
	int ok = 1;
	struct World *world = NULL;

	// initialize renderer
	if (!renderer_init(SCREEN_WIDTH, SCREEN_HEIGHT)) {
		return EXIT_FAILURE;
	}

	// create a render list
	struct RenderList *rndr_list = render_list_new();

	// create Lua script environment
	struct ScriptEnv *env = script_env_new();
	if (!env) {
		ok = 0;
		goto cleanup;
	}

	if (!(ok = load_resources())) {
		goto cleanup;
	}

	if (!(world = world_new())) {
		ok = 0;
		goto cleanup;
	}

	// initialize script environment and perform initial tick
	if (!script_env_init(env, world) ||
	    !script_env_load_file(env, "data/scripts/game.lua") ||
	    !script_env_tick(env)) {
		ok = 0;
		goto cleanup;
	}

	int run = 1;
	Uint32 last_update = SDL_GetTicks();
	float tick = 0, time_acc = 0;
	unsigned frame_count = 0;
	while (ok && run) {
		// compute timers and counters
		Uint32 now = SDL_GetTicks();
		float dt = (now - last_update) / 1000.0f;
		last_update = now;
		tick += dt;
		time_acc += dt;
		frame_count++;

		// handle input
		SDL_Event evt;
		while (SDL_PollEvent(&evt)) {
			if (evt.type == SDL_KEYDOWN || evt.type == SDL_KEYUP) {
				switch (evt.key.keysym.sym) {
				case SDLK_q:
				case SDLK_ESCAPE:
					run = 0;
				}
				run &= handle_key(&evt, world);
			} else if (evt.type == SDL_QUIT) {
				run = 0;
			}
		}

		// update the world
		run &= world_update(world, dt);

		// notify script environment
		while (tick >= TICK) {
			tick -= TICK;
			ok &= script_env_tick(env);
		}

		// render!
		now = SDL_GetTicks();
		renderer_clear();
		render_world(rndr_list, world);
		render_ui(rndr_list);
		render_list_exec(rndr_list);
		renderer_present();
		Uint32 render_time = SDL_GetTicks() - now;

		// each second, update the stats
		if (time_acc >= 1.0) {
			time_acc -= 1.0;

			// update fps
			char *str = string_fmt("FPS: %d", frame_count);
			text_set_string(fps_text, str);
			free(str);
			frame_count = 0;

			// update render time
			str = string_fmt("Render time: %dms", render_time);
			text_set_string(render_time_text, str);
			free(str);
		}
	}

cleanup:
	script_env_destroy(env);
	world_destroy(world);
	cleanup_resources();
	renderer_shutdown();

 	ok &= !error_is_set();
	if (!ok) {
		error_dump(stdout);
	}


	printf(ok ? "Bye!\n" : "Oops!\n");
	return !(ok == 1);
}