#include "error.h"
#include "font.h"
#include "game.h"
#include "matlib.h"
#include "memory.h"
#include "script.h"
#include "shader.h"
#include "sprite.h"
#include <GL/glew.h>
#include <SDL.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RENDER_LIST_MAX_LEN 1000

/**
 * Render node.
 */
struct RenderNode {
	GLuint vao;
	GLuint texture;
	Vec size;
	Mat transform;
};

/**
 * Render list.
 */
struct RenderList {
	struct RenderNode nodes[RENDER_LIST_MAX_LEN];
	size_t len;
};

/**
 * Rendering pipeline.
 */
struct Pipeline {
	Mat projection;
	struct Shader *shader;
	struct ShaderUniform u_texture;
	struct ShaderUniform u_size;
	struct ShaderUniform u_transform;
};

struct Renderer {
	SDL_Window *win;
	SDL_GLContext *ctx;
	struct Pipeline pipeline;
};

/*** RESOURCES ***/
static struct Sprite *spr_player = NULL;
static struct Sprite *spr_enemy_01 = NULL;
static struct Sprite *spr_asteroid_01 = NULL;
static struct Sprite *spr_projectile_01 = NULL;
static struct Font *font = NULL;

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
			fprintf(stderr, "failed to load `%s`\n", sprite_files[i]);
			return 0;
		}
		printf("loaded sprite `%s`\n", sprite_files[i]);
	}

	// load font
	font = font_from_file("data/fonts/kenvector_future_thin.ttf", 14);
	if (!font) {
		return 0;
	}

	return 1;
}

static void
cleanup_resources(void)
{
	font_destroy(font);
	sprite_destroy(spr_player);
	sprite_destroy(spr_asteroid_01);
	sprite_destroy(spr_projectile_01);
}

/**
 * Adds the given sprite to the render list.
 */
static void
render_list_add_sprite(
	struct RenderList *list,
	const struct Sprite *spr,
	float x,
	float y,
	float angle
);

/**
 * Execute a render list.
 */
static int
render_list_exec(struct RenderList *list, struct Renderer *rndr);

/**
 * Initializes renderer and various subsystems.
 */
static int
renderer_init(struct Renderer *rndr, unsigned width, unsigned height);

/**
 * Cleans-up and shuts down everything.
 */
static void
renderer_shutdown(struct Renderer *rndr);

/**
 * Keyboard events handler callback.
 */
static int
handle_key(const SDL_Event *key_evt, struct World *world);

/**
 * Add a render node to render queue.
 */
void
pipeline_add_node(struct Pipeline *pipelne, const struct RenderNode *node);

static int
renderer_init(struct Renderer *rndr, unsigned width, unsigned height) {
	// initialize SDL video subsystem
	if (!SDL_WasInit(SDL_INIT_VIDEO) && SDL_Init(SDL_INIT_VIDEO) != 0) {
		fprintf(stderr, "failed to initialize SDL: %s", SDL_GetError());
		return 0;
	}

	// create window
	rndr->win = SDL_CreateWindow(
		"Shooter",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		width,
		height,
		SDL_WINDOW_OPENGL
	);
	if (!rndr->win) {
		fprintf(stderr, "failed to create OpenGL window\n");
		goto error;
	}

	// initialize OpenGL context
	SDL_GL_SetAttribute(
		SDL_GL_CONTEXT_PROFILE_MASK,
		SDL_GL_CONTEXT_PROFILE_CORE
	);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	rndr->ctx = SDL_GL_CreateContext(rndr->win);
	if (!rndr->ctx) {
		fprintf(stderr, "failed to initialize OpenGL context\n");
		goto error;
	}

	// initialize GLEW
	glewExperimental = GL_TRUE;
	if (glewInit() != 0) {
		fprintf(stderr, "failed to initialize GLEW");
		goto error;
	}
	glGetError(); // silence any errors produced during GLEW initialization

	printf("OpenGL version: %s\n", glGetString(GL_VERSION));
	printf("GLSL version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	printf("GLEW version: %s\n", glewGetString(GLEW_VERSION));

	// initialize OpenGL state machine
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// initialize pipeline
	memset(&rndr->pipeline, 0, sizeof(struct Pipeline));

	// initialize projection matrix
	mat_ortho(
		&rndr->pipeline.projection,
		-(float)width / 2,
		(float)width / 2,
		(float)height / 2,
		-(float)height / 2,
		0,
		100
	);

	// load and compile the shader
	const char *uniform_names[] = {
		"tex",
		"size",
		"transform",
		NULL
	};
	struct ShaderUniform *uniforms[] = {
		&rndr->pipeline.u_texture,
		&rndr->pipeline.u_size,
		&rndr->pipeline.u_transform,
		NULL
	};
	rndr->pipeline.shader = shader_compile(
		"data/shaders/sprite.vert",
		"data/shaders/sprite.frag",
		uniform_names,
		uniforms,
		NULL,
		NULL
	);
	if (!rndr->pipeline.shader || !shader_bind(rndr->pipeline.shader)) {
		fprintf(
			stderr,
			"failed to initialize rendering pipeline\n"
		);
		goto error;
	}

	return 1;

error:
	renderer_shutdown(rndr);
	return 0;
}

static void
renderer_shutdown(struct Renderer *rndr)
{
	shader_free(rndr->pipeline.shader);

	if (rndr->ctx) {
		SDL_GL_DeleteContext(rndr->ctx);
	}
	if (rndr->win) {
		SDL_DestroyWindow(rndr->win);
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
	}
	return 1;
}

static void
render_list_add_sprite(
	struct RenderList *list,
	const struct Sprite *spr,
	float x,
	float y,
	float angle
) {
	assert(list->len < RENDER_LIST_MAX_LEN);
	struct RenderNode *node = &list->nodes[list->len++];
	node->vao = spr->vao;
	node->texture = spr->texture;
	node->size = vec(spr->width, spr->height, 0, 0);
	Mat t, r;
	mat_ident(&t);
	mat_translate(&t, x, -y, 0);

	mat_ident(&r);
	mat_translate(&r, -spr->width / 2, spr->height / 2, 0);
	mat_rotate(&r, 0, 0, 1, angle);

	mat_mul(&t, &r, &node->transform);
}

static int
render_list_exec(struct RenderList *list, struct Renderer *rndr)
{
	Mat mvp;
	GLuint texture_unit = 0;
	glActiveTexture(GL_TEXTURE0 + texture_unit);

	int ok = 1;
	for (size_t i = 0; i < list->len; i++) {
		struct RenderNode *node = &list->nodes[i];

		// configure size
		ok &= shader_uniform_set(
			&rndr->pipeline.u_size,
			1,
			&node->size
		);

		// configure transform
		mat_mul(&rndr->pipeline.projection, &node->transform, &mvp);
		ok &= shader_uniform_set(
			&rndr->pipeline.u_transform,
			1,
			&mvp
		);

		// configure texture sampler
		ok &= shader_uniform_set(
			&rndr->pipeline.u_texture,
			1,
			&texture_unit
		);

		// render
		glBindTexture(GL_TEXTURE_RECTANGLE, node->texture);
		glBindVertexArray(node->vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		ok &= glGetError() == GL_NO_ERROR;

		if (!ok) {
			break;
		}
	}

	list->len = 0;

	return ok;
}

int
render_world(struct World *world, struct RenderList *rndr_list)
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

	return 1;
}

int
main(int argc, char *argv[])
{
	int ok = 1;
	struct World *world = NULL;

	struct Renderer rndr;
	struct RenderList rndr_list = { .len = 0 };
	if (!renderer_init(&rndr, SCREEN_WIDTH, SCREEN_HEIGHT)) {
		return EXIT_FAILURE;
	}

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
	float tick = 0;
	while (ok && run) {
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

		// compute delta time and update the world
		Uint32 now = SDL_GetTicks();
		float dt = (now - last_update) / 1000.0f;
		last_update = now;
		tick += dt;

		// notify script environment
		while (tick >= TICK) {
			tick -= TICK;
			ok &= script_env_tick(env);
		}

		// update the world
		run &= world_update(world, dt);

		// render!
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		render_world(world, &rndr_list);
		render_list_exec(&rndr_list, &rndr);
		SDL_GL_SwapWindow(rndr.win);
	}

cleanup:
	script_env_destroy(env);
	world_destroy(world);
	cleanup_resources();
	renderer_shutdown(&rndr);

 	ok &= !error_is_set();
	if (!ok) {
		error_dump(stdout);
	}


	printf(ok ? "Bye!\n" : "Oops!\n");
	return !(ok == 1);
}