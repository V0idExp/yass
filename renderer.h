#pragma once

#include <GL/glew.h>
#include <SDL.h>

struct Sprite;
struct Text;
struct Texture;

/**
 * Render list.
 */
struct RenderList;

/**
 * Create a render list.
 */
struct RenderList*
render_list_new(void);

/**
 * Destroy a render list.
 */
void
render_list_destroy(struct RenderList *list);

/**
 * Add a sprite to render list.
 */
void
render_list_add_sprite(
	struct RenderList *list,
	const struct Sprite *spr,
	float x,
	float y,
	float z,
	float angle
);

/**
 * Add a text to render list.
 */
void
render_list_add_text(
	struct RenderList *list,
	const struct Text *txt,
	float x,
	float y,
	float z
);

/**
 * Add a image to render list.
 */
void
render_list_add_image(
	struct RenderList *list,
	const struct Texture *tex,
	float x,
	float y,
	float z,
	float w,
	float h
);

/**
 * Execute a render list.
 */
int
render_list_exec(struct RenderList *list);

/**
 * Initialize rendering system.
 */
int
renderer_init(unsigned width, unsigned height);

/**
 * Clean-up and shut down renderer.
 */
void
renderer_shutdown(void);

/**
 * Clear screen.
 */
void
renderer_clear(void);

/**
 * Present buffered contents to the screen.
 */
void
renderer_present(void);