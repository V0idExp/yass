#include "error.h"
#include "game.h"
#include "renderer.h"
#include "script.h"
#include "sprite.h"
#include "state.h"
#include "ui.h"
#include <GL/glew.h>
#include <SDL.h>
#include <assert.h>
#include <stdlib.h>

/*** GLOBAL STATE ***/
static struct State state = {
	.game_paused = 0,
	.show_upgrade_shop = 0,

	.credits = 0,
	.hitpoints = 0,

	.fps = 0,
	.render_time = 0
};

/*** RESOURCES ***/
static struct Sprite *spr_player = NULL;
static struct Sprite *spr_enemy_01 = NULL;
static struct Sprite *spr_asteroid_01 = NULL;
static struct Sprite *spr_projectile_01 = NULL;

// SPRITES
static const struct {
	const char *file;
	struct Sprite **var;
} sprites[] = {
	{ "data/art/playerShip1_blue.png", &spr_player },
	{ "data/art/Enemies/enemyBlack2.png", &spr_enemy_01 },
	{ "data/art/Meteors/meteorGrey_small2.png", &spr_asteroid_01 },
	{ "data/art/Lasers/laserBlue07.png", &spr_projectile_01 },
	{ NULL }
};

static int
load_resources(void)
{
	// load sprites
	for (unsigned i = 0; sprites[i].file != NULL; i++) {
		if (!(*sprites[i].var = sprite_from_file(sprites[i].file))) {
			fprintf(
				stderr,
				"failed to load sprite `%s`\n",
				sprites[i].file
			);
			return 0;
		}
		printf("loaded sprite `%s`\n", sprites[i].file);
	}

	return 1;
}

static void
cleanup_resources(void)
{
	// destroy sprites
	for (unsigned i = 0; sprites[i].file; i++) {
		sprite_destroy(*sprites[i].var);
	}
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

		if (key_evt->key.keysym.sym == SDLK_u) {
			state.show_upgrade_shop = !state.show_upgrade_shop;
		}
	}
	return 1;
}

static int
handle_mouse(const SDL_Event *mouse_evt)
{
	if (mouse_evt->button.button == SDL_BUTTON_LEFT &&
	    mouse_evt->button.clicks == 1) {
		return ui_handle_click(
			mouse_evt->button.x,
			mouse_evt->button.y
		);
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

	if (!(ok = (load_resources() && ui_load()))) {
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
		// compute time delta, update timers and counters
		Uint32 now = SDL_GetTicks();
		float dt = (now - last_update) / 1000.0f;
		last_update = now;
		time_acc += dt;
		if (time_acc >= 1.0) {
			time_acc -= 1.0;

			// update fps
			state.fps = frame_count;
			frame_count = 0;
		} else {
			frame_count++;
		}

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
			} else if (evt.type == SDL_MOUSEBUTTONUP) {
				run &= handle_mouse(&evt);
			}
		}

		// update the game
		state.game_paused = state.show_upgrade_shop;
		if (!state.game_paused) {
			// update the world
			run &= world_update(world, dt);

			// notify script environment
			tick += dt;
			while (tick >= TICK) {
				tick -= TICK;
				ok &= script_env_tick(env);
			}

			// update the state
			state.credits = world->player.credits;
			state.hitpoints = world->player.hitpoints;
		}

		// update the UI
		ok &= ui_update(&state, dt);

		// render and measure rendering time
		now = SDL_GetTicks();
		renderer_clear();
		render_world(rndr_list, world);
		ui_render(rndr_list);
		render_list_exec(rndr_list);
		renderer_present();
		state.render_time = SDL_GetTicks() - now;
	}

cleanup:
	script_env_destroy(env);
	world_destroy(world);
	ui_cleanup();
	cleanup_resources();
	renderer_shutdown();

 	ok &= !error_is_set();
	if (!ok) {
		error_dump(stdout);
	}

	printf(ok ? "Bye!\n" : "Oops!\n");
	return !(ok == 1);
}