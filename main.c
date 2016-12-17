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
#include "texture.h"
#include "widget.h"
#include <GL/glew.h>
#include <SDL.h>
#include <assert.h>
#include <stdlib.h>

/*** RESOURCES ***/
static struct Sprite *spr_player = NULL;
static struct Sprite *spr_enemy_01 = NULL;
static struct Sprite *spr_asteroid_01 = NULL;
static struct Sprite *spr_projectile_01 = NULL;
static struct Font *font_dbg = NULL;
static struct Font *font_hud = NULL;
static struct Text *fps_text = NULL;
static struct Text *render_time_text = NULL;
static struct Text *credits_text = NULL;
static struct Widget *hp_bar = NULL;
static struct Widget *hp_bar_bg = NULL;
static struct Texture *tex_hp_bar_green = NULL;
static struct Texture *tex_hp_bar_bg = NULL;

// TEXTURES
static const struct TextureRes {
	const char *file;
	struct Texture **var;
} textures[] = {
	{ "data/art/UI/squareGreen.png", &tex_hp_bar_green },
	{ "data/art/UI/squareRed.png", &tex_hp_bar_bg },
	{ NULL }
};

// SPITES
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

// FONTS
static const struct {
	const char *file;
	unsigned size;
	struct Font **var;
} fonts[] = {
	{ "data/fonts/courier.ttf", 16, &font_dbg },
	{ "data/fonts/kenvector_future_thin.ttf", 16, &font_hud },
	{ NULL }
};

static int
load_resources(void)
{
	// load textures
	for (unsigned i = 0; textures[i].file != NULL; i++) {
		struct TextureRes res = textures[i];
		if (!(*res.var = texture_from_file(res.file))) {
			fprintf(
				stderr,
				"failed to load texture `%s`\n",
				res.file
			);
			return 0;
		}
		printf("loaded texure `%s`\n", res.file);
	}

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

	// load fonts
	for (unsigned i = 0; fonts[i].file != NULL; i++) {
		if (!(*fonts[i].var = font_from_file(fonts[i].file, fonts[i].size))) {
			fprintf(
				stderr,
				"failed to load font `%s`\n",
				fonts[i].file
			);
			return 0;
		}
		printf("loaded font `%s`\n", fonts[i].file);
	}

	// create text renderables
	fps_text = text_new(font_dbg);
	render_time_text = text_new(font_dbg);
	credits_text = text_new(font_hud);
	if (!fps_text || !render_time_text || !credits_text) {
		return 0;
	}

	// create widgets
	hp_bar = widget_new();
	if (!hp_bar) {
		return 0;
	}
	hp_bar->texture = tex_hp_bar_green;
	hp_bar->width = 200;
	hp_bar->height = 26;
	hp_bar->border.left = 6;
	hp_bar->border.right = 6;

	hp_bar_bg = widget_new();
	if (!hp_bar_bg) {
		return 0;
	}
	hp_bar_bg->texture = tex_hp_bar_bg;
	hp_bar_bg->width = 200;
	hp_bar_bg->height = 26;
	hp_bar_bg->border.left = 6;
	hp_bar_bg->border.right = 6;

	return 1;
}

static void
cleanup_resources(void)
{
	widget_destroy(hp_bar_bg);
	widget_destroy(hp_bar);
	text_destroy(fps_text);
	text_destroy(render_time_text);
	text_destroy(credits_text);

	// destroy fonts
	for (unsigned i = 0; fonts[i].file; i++) {
		font_destroy(*fonts[i].var);
	}

	// destroy sprites
	for (unsigned i = 0; sprites[i].file; i++) {
		sprite_destroy(*sprites[i].var);
	}

	// destroy textures
	for (unsigned i = 0; textures[i].file; i++) {
		texture_destroy(*textures[i].var);
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

static void
render_ui(struct RenderList *rndr_list)
{
	// render FPS indicator
	render_list_add_text(
		rndr_list,
		fps_text,
		-SCREEN_WIDTH / 2,
		-SCREEN_HEIGHT / 2 + 60
	);

	// render render time indicator
	render_list_add_text(
		rndr_list,
		render_time_text,
		-SCREEN_WIDTH / 2,
		-SCREEN_HEIGHT / 2 + 80
	);

	// render credits counter
	render_list_add_text(
		rndr_list,
		credits_text,
		SCREEN_WIDTH / 2 - 150,
		-SCREEN_HEIGHT / 2 + 20
	);

	// render hitpoints widget
	render_list_add_widget(
		rndr_list,
		hp_bar_bg,
		20,
		25 - hp_bar_bg->height / 2
	);
	render_list_add_widget(
		rndr_list,
		hp_bar,
		20,
		25 - hp_bar->height / 2
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
	int current_credits = -1;
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

		// update credits text
		if (world->player.credits != current_credits) {
			current_credits = world->player.credits;
			text_set_fmt(credits_text, "Credits: %d$", current_credits);
		}

		// update hitpoints widget
		hp_bar->width = 200.0 * world->player.hitpoints / PLAYER_INITIAL_HITPOINTS;

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
			text_set_fmt(fps_text, "FPS: %d", frame_count);
			frame_count = 0;

			// update render time
			text_set_fmt(
				render_time_text,
				"Render time: %dms",
				render_time
			);
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