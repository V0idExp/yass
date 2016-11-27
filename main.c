#include <GL/glew.h>
#include <SDL.h>
#include <assert.h>
#include <stdio.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 800

/**
 * Initialize SDL and various subsystems.
 */
static int
init(unsigned width, unsigned height, SDL_Window **win, SDL_GLContext **ctx);

/**
 * Clean-up and shutdown everything.
 */
static void
shutdown(SDL_Window *win, SDL_GLContext *ctx);

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

int main(int argc, char *argv[])
{
	SDL_Window *win = NULL;
	SDL_GLContext *ctx = NULL;
	if (!init(SCREEN_WIDTH, SCREEN_HEIGHT, &win, &ctx)) {
		return EXIT_FAILURE;
	}

	int run = 1;
	while (run) {
		SDL_Event evt;
		if (SDL_PollEvent(&evt)) {
			if (evt.type == SDL_KEYDOWN) {
				switch (evt.key.keysym.sym) {
				case SDLK_q:
				case SDLK_ESCAPE:
					run = 0;
				}
			} else if (evt.type == SDL_QUIT) {
				run = 0;
			}
		}
	}

	shutdown(win, ctx);
	return 0;
}