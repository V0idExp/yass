#include <GL/glew.h>
#include <SDL.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "matlib.h"
#include "memory.h"
#include "shader.h"
#include "sprite.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 800
#define RENDER_LIST_MAX_LEN 1000
#define MAX_ASTEROIDS 15
#define MAX_PROJECTILES 20
#define PLAYER_INITIAL_SPEED 100.0  // units/second
#define PLAYER_SHOOT_RATE 1.0  // projectiles/second
#define PLAYER_PROJECTILE_INITIAL_SPEED 400  // units/second
#define PLAYER_PROJECTILE_TTL 5.0  // seconds

/**
 * Player action bits.
 */
enum {
	MOVE_LEFT = 1,
	MOVE_RIGHT = 1 << 1,
	MOVE_UP = 1 << 2,
	MOVE_DOWN = 1 << 3,
	SHOOT = 1 << 4
};

/**
 * Player.
 */
struct Player {
	float x, y;     // position
	int actions;    // actions bitmask
	float speed;    // speed in units/second
	float shoot_cooldown;
	struct Sprite *sprite;
};

/**
 * Asteroid.
 */
struct Asteroid {
	float x, y;
	float xvel, yvel;
	float rot;
	float rot_speed;
	struct Sprite *sprite;
};

/**
 * Projectile.
 */
struct Projectile {
	float x, y;
	float xvel, yvel;
	float ttl;
	struct Sprite *sprite;
};

/**
 * World container.
 *
 * This struct holds all the objects which make up the game.
 */
struct World {
	struct Player player;
	struct Asteroid asteroids[MAX_ASTEROIDS];
	struct Projectile projectiles[MAX_PROJECTILES];
};

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
static struct Sprite *spr_asteroid_01 = NULL;
static struct Sprite *spr_projectile_01 = NULL;

static int
load_resources(void)
{
	const char *sprite_files[] = {
		"data/playerShip1_blue.png",
		"data/meteorGrey_small2.png",
		"data/laserBlue07.png",
		NULL
	};
	struct Sprite **sprites[] = {
		&spr_player,
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

	return 1;
}

static void
cleanup_resources(void)
{
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
 * Creates and initializes game world.
 */
struct World*
world_new(void);

/**
 * Destroys game world.
 */
static void
world_destroy(struct World *w);

/**
 * Add a projectile into the world.
 */
static void
world_add_projectile(struct World *w, const struct Projectile *projectile);

/**
 * Update the world by given delta time.
 */
static int
world_update(struct World *world, float dt);

/**
 * Renders the world.
 */
static int
world_render(struct World *world, struct RenderList *rndr_list);

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
		"data/rect.vert",
		"data/rect.frag",
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
		act = MOVE_LEFT;
		break;
	case SDLK_d:
	case SDLK_RIGHT:
		act = MOVE_RIGHT;
		break;
	case SDLK_w:
	case SDLK_UP:
		act = MOVE_UP;
		break;
	case SDLK_s:
	case SDLK_DOWN:
		act = MOVE_DOWN;
		break;
	case SDLK_SPACE:
		act = SHOOT;
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

struct World*
world_new(void)
{
	struct World *w = make(struct World);
	if (!w) {
		return NULL;
	}

	// initialize player
	w->player.sprite = spr_player;
	w->player.speed = PLAYER_INITIAL_SPEED;

	// initialize asteroids
	for (int i = 0; i < MAX_ASTEROIDS; i++) {
		struct Asteroid *ast = &w->asteroids[i];
		ast->x = random() % SCREEN_WIDTH - SCREEN_WIDTH / 2;
		ast->y = random() % SCREEN_HEIGHT - SCREEN_HEIGHT / 2;
		ast->xvel = ((random() % 2) ? -1 : 1) * (random() % 25);
		ast->yvel = ((random() % 2) ? -1 : 1) * (random() % 25);
		ast->sprite = spr_asteroid_01;
		ast->rot_speed = ((random() % 2) ? -1 : 1) * 2.0 * M_PI / (2 + random() % 6);
	}

	return w;
}

static void
world_destroy(struct World *w)
{
	if (w) {
		free(w);
	}
}

static void
world_add_projectile(struct World *w, const struct Projectile *projectile)
{
	size_t oldest = 0;
	float oldest_ttl = PLAYER_PROJECTILE_TTL;
	for (size_t i = 0; i < MAX_PROJECTILES; i++) {
		if (w->projectiles[i].ttl <= 0) {
			w->projectiles[i] = *projectile;
			return;
		} else if (w->projectiles[i].ttl < oldest_ttl) {
			oldest_ttl = w->projectiles[i].ttl;
			oldest = i;
		}
	}
	w->projectiles[oldest] = *projectile;
}

static int
world_update(struct World *world, float dt)
{
	// update asteroids
	for (int i = 0; i < MAX_ASTEROIDS; i++) {
		struct Asteroid *ast = &world->asteroids[i];
		ast->x += ast->xvel * dt;
		ast->y += ast->yvel * dt;
		ast->rot += ast->rot_speed * dt;
		if (ast->rot >= M_PI * 2) {
			ast->rot -= M_PI * 2;
		}
	}

	// update projectiles
	for (int i = 0; i < MAX_PROJECTILES; i++) {
		struct Projectile *prj = &world->projectiles[i];
		if (prj->ttl > 0) {
			prj->x += prj->xvel * dt;
			prj->y += prj->yvel * dt;
			prj->ttl -= dt;
		}
	}

	// update player position
	float distance = dt * world->player.speed;
	if (world->player.actions & MOVE_LEFT) {
		world->player.x -= distance;
	}
	if (world->player.actions & MOVE_RIGHT) {
		world->player.x += distance;
	}
	if (world->player.actions & MOVE_UP) {
		world->player.y -= distance;
	}
	if (world->player.actions & MOVE_DOWN) {
		world->player.y += distance;
	}

	// handle shooting
	world->player.shoot_cooldown -= dt;
	if (world->player.actions & SHOOT &&
	    world->player.shoot_cooldown <= 0) {
		// reset cooldown timer
		world->player.shoot_cooldown = 1.0 / PLAYER_SHOOT_RATE;
		// shoot
		struct Projectile p = {
			.x = world->player.x,
			.y = world->player.y,
			.xvel = 0,
			.yvel = -PLAYER_PROJECTILE_INITIAL_SPEED,
			.ttl = PLAYER_PROJECTILE_TTL,
			.sprite = spr_projectile_01
		};
		world_add_projectile(world, &p);
	}

	return 1;
}

static int
world_render(struct World *world, struct RenderList *rndr_list)
{
	render_list_add_sprite(
		rndr_list,
		world->player.sprite,
		world->player.x,
		world->player.y,
		0.0f
	);

	for (int i = 0; i < MAX_ASTEROIDS; i++) {
		render_list_add_sprite(
			rndr_list,
			world->asteroids[i].sprite,
			world->asteroids[i].x,
			world->asteroids[i].y,
			world->asteroids[i].rot
		);
	}

	for (int i = 0; i < MAX_PROJECTILES; i++) {
		struct Projectile *prj = &world->projectiles[i];
		if (prj->ttl > 0) {
			render_list_add_sprite(
				rndr_list,
				prj->sprite,
				prj->x,
				prj->y,
				0
			);
		}
	}

	return 1;
}

int
main(int argc, char *argv[])
{
	struct Renderer rndr;
	struct RenderList rndr_list = { .len = 0 };
	if (!renderer_init(&rndr, SCREEN_WIDTH, SCREEN_HEIGHT)) {
		return EXIT_FAILURE;
	}

	struct World *world = NULL;

	int ok = load_resources();
	if (!ok) {
		goto cleanup;
	}

	if (!(world = world_new())) {
		ok = 0;
		goto cleanup;
	}

	int run = 1;
	Uint32 last_update = SDL_GetTicks();
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
		run &= world_update(world, dt);

		// render!
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		world_render(world, &rndr_list);
		render_list_exec(&rndr_list, &rndr);
		SDL_GL_SwapWindow(rndr.win);
	}

cleanup:
	world_destroy(world);
	cleanup_resources();
	renderer_shutdown(&rndr);
	return !(ok == 1);
}