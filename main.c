#include <GL/glew.h>
#include <SDL.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 800
#define PLAYER_INITIAL_SPEED 20.0  // units/second

/**
 * Player movement directions bitmask.
 */
enum {
	MOVE_LEFT = 1,
	MOVE_RIGHT = 1 << 1,
	MOVE_UP = 1 << 2,
	MOVE_DOWN = 1 << 3
};

/**
 * Player.
 */
struct Player {
	float x, y;     // position
	int move_dirs;  // movement directions bitfield
	float speed;    // speed in units/second
};

/**
 * World container.
 *
 * This struct holds all the objects which make up the game.
 */
struct World {
	struct Player player;
};

/**
 * Allocates given amount of bytes and zeroes it.
 */
static void*
alloc0(size_t size);

/**
 * Allocates given type and initializes it to 0.
 */
#define make(t) (alloc0(sizeof(t)))

/**
 * Initializes SDL and various subsystems.
 */
static int
init(unsigned width, unsigned height, SDL_Window **win, SDL_GLContext **ctx);

/**
 * Cleans-up and shuts down everything.
 */
static void
shutdown(SDL_Window *win, SDL_GLContext *ctx);

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
 * Update the world by given delta time.
 */
static int
world_update(struct World *world, float dt);

static void*
alloc0(size_t size)
{
	void *bytes = malloc(size);
	if (!size) {
		return NULL;
	}
	memset(bytes, 0, size);
	return bytes;
}

static int
init(
	unsigned width,
	unsigned height,
	SDL_Window **win,
	SDL_GLContext **ctx
) {
	assert(win);
	assert(ctx);
	*win = NULL;
	*ctx = NULL;

	// initialize SDL video subsystem
	if (!SDL_WasInit(SDL_INIT_VIDEO) && SDL_Init(SDL_INIT_VIDEO) != 0) {
		fprintf(stderr, "failed to initialize SDL: %s", SDL_GetError());
		return 0;
	}

	// create window
	*win = SDL_CreateWindow(
		"Shooter",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		width,
		height,
		SDL_WINDOW_OPENGL
	);
	if (!*win) {
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

	*ctx = SDL_GL_CreateContext(*win);
	if (!*ctx) {
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

	return 1;

error:
	shutdown(*win, *ctx);
	return 0;
}

static void
shutdown(SDL_Window *win, SDL_GLContext *ctx)
{
	if (ctx) {
		SDL_GL_DeleteContext(ctx);
	}
	if (win) {
		SDL_DestroyWindow(win);
	}
}

static int
handle_key(const SDL_Event *key_evt, struct World *world)
{
	// handle player movement keys
	int dir = 0;
	switch (key_evt->key.keysym.sym) {
	case SDLK_a:
	case SDLK_LEFT:
		dir = MOVE_LEFT;
		break;
	case SDLK_d:
	case SDLK_RIGHT:
		dir = MOVE_RIGHT;
		break;
	case SDLK_w:
	case SDLK_UP:
		dir = MOVE_UP;
		break;
	case SDLK_s:
	case SDLK_DOWN:
		dir = MOVE_DOWN;
		break;
	}

	if (key_evt->type == SDL_KEYUP) {
		world->player.move_dirs &= ~dir;
	} else {
		world->player.move_dirs |= dir;
	}
	return 1;
}

struct World*
world_new(void)
{
	struct World *w = make(struct World);
	if (!w) {
		return NULL;
	}

	// initialize player
	w->player.speed = PLAYER_INITIAL_SPEED;

	return w;
}

static void
world_destroy(struct World *w)
{
	if (w) {
		free(w);
	}
}

static int
world_update(struct World *world, float dt)
{
	// update player position
	float distance = dt * world->player.speed;
	if (world->player.move_dirs & MOVE_LEFT) {
		world->player.x -= distance;
	} else if (world->player.move_dirs & MOVE_RIGHT) {
		world->player.x += distance;
	} else if (world->player.move_dirs & MOVE_UP) {
		world->player.y -= distance;
	} else if (world->player.move_dirs & MOVE_DOWN) {
		world->player.y += distance;
	}

	return 1;
}

int main(int argc, char *argv[])
{
	SDL_Window *win = NULL;
	SDL_GLContext *ctx = NULL;
	if (!init(SCREEN_WIDTH, SCREEN_HEIGHT, &win, &ctx)) {
		return EXIT_FAILURE;
	}

	int ok = 1;

	struct World *world = world_new();
	if (!world) {
		ok = 0;
		goto cleanup;
	}

	int run = 1;
	Uint32 last_update = SDL_GetTicks();
	while (run) {
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
	}

cleanup:
	world_destroy(world);
	shutdown(win, ctx);
	return !(ok == 1);
}