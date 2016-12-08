#include "error.h"
#include "font.h"
#include "matlib.h"
#include "memory.h"
#include "renderer.h"
#include "shader.h"
#include "sprite.h"
#include "text.h"
#include "texture.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define RENDER_LIST_MAX_LEN 1000
#define SPRITE_TEXTURE_UNIT 0
#define TEXT_GLYPH_TEXTURE_UNIT 1
#define TEXT_ATLAS_TEXTURE_UNIT 2

enum {
	RENDER_NODE_SPRITE,
	RENDER_NODE_TEXT
};

static struct Renderer {
	int initialized;
	SDL_Window *win;
	SDL_GLContext *ctx;
	Mat projection;
	struct {
		struct Shader *shader;
		struct ShaderUniform u_texture;
		struct ShaderUniform u_size;
		struct ShaderUniform u_transform;
	} sprite_pipeline;
	struct {
		struct Shader *shader;
		struct ShaderUniform u_transform;
		struct ShaderUniform u_glyph_texture;
		struct ShaderUniform u_atlas_texture;
		struct ShaderUniform u_atlas_offset;
	} text_pipeline;
} rndr = { 0, NULL, NULL };

struct RenderNode {
	int type;
	Mat transform;
	union {
		struct Sprite *sprite;
		struct Text *text;
	};
};

struct RenderList {
	struct RenderNode nodes[RENDER_LIST_MAX_LEN];
	size_t len;
};

static int
init_sprite_pipeline(void)
{
	// load and compile the shader
	const char *uniform_names[] = {
		"tex",
		"size",
		"transform",
		NULL
	};
	struct ShaderUniform *uniforms[] = {
		&rndr.sprite_pipeline.u_texture,
		&rndr.sprite_pipeline.u_size,
		&rndr.sprite_pipeline.u_transform,
		NULL
	};
	rndr.sprite_pipeline.shader = shader_compile(
		"data/shaders/sprite.vert",
		"data/shaders/sprite.frag",
		uniform_names,
		uniforms,
		NULL,
		NULL
	);
	if (!rndr.sprite_pipeline.shader) {
		fprintf(
			stderr,
			"failed to initialize rendering pipeline\n"
		);
		return 0;
	}
	return 1;
}

static int
init_text_pipeline(void)
{
	// load and compile the shader
	const char *uniform_names[] = {
		"glyph_tex",
		"atlas_tex",
		"atlas_offset",
		"transform",
		NULL
	};
	struct ShaderUniform *uniforms[] = {
		&rndr.text_pipeline.u_glyph_texture,
		&rndr.text_pipeline.u_atlas_texture,
		&rndr.text_pipeline.u_atlas_offset,
		&rndr.text_pipeline.u_transform,
		NULL
	};
	rndr.text_pipeline.shader = shader_compile(
		"data/shaders/text.vert",
		"data/shaders/text.frag",
		uniform_names,
		uniforms,
		NULL,
		NULL
	);
	if (!rndr.text_pipeline.shader) {
		fprintf(
			stderr,
			"failed to initialize rendering pipeline\n"
		);
		return 0;
	}
	return 1;
}

int
renderer_init(unsigned width, unsigned height)
{
	assert(!rndr.initialized);
	memset(&rndr, 0, sizeof(struct Renderer));

	// initialize SDL video subsystem
	if (!SDL_WasInit(SDL_INIT_VIDEO) && SDL_Init(SDL_INIT_VIDEO) != 0) {
		fprintf(stderr, "failed to initialize SDL: %s", SDL_GetError());
		error(ERR_SDL);
		return 0;
	}

	// create window
	rndr.win = SDL_CreateWindow(
		"Shooter",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		width,
		height,
		SDL_WINDOW_OPENGL
	);
	if (!rndr.win) {
		fprintf(stderr, "failed to create OpenGL window\n");
		error(ERR_SDL);
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

	rndr.ctx = SDL_GL_CreateContext(rndr.win);
	if (!rndr.ctx) {
		fprintf(stderr, "failed to initialize OpenGL context\n");
		error(ERR_SDL);
		goto error;
	}
	SDL_GL_SetSwapInterval(0);

	// initialize GLEW
	glewExperimental = GL_TRUE;
	if (glewInit() != 0) {
		fprintf(stderr, "failed to initialize GLEW");
		error(ERR_OPENGL);
		goto error;
	}
	glGetError(); // silence any errors produced during GLEW initialization

	printf("OpenGL version: %s\n", glGetString(GL_VERSION));
	printf("GLSL version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
	printf("GLEW version: %s\n", glewGetString(GLEW_VERSION));

	// initialize OpenGL state machine
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// initialize projection matrix
	mat_ortho(
		&rndr.projection,
		-(float)width / 2,
		(float)width / 2,
		(float)height / 2,
		-(float)height / 2,
		0,
		100
	);

	rndr.initialized = (
		init_sprite_pipeline() &&
		init_text_pipeline()
	);

	if (!rndr.initialized) {
		goto error;
	}

	return 1;

error:
	renderer_shutdown();
	return 0;
}

void
renderer_shutdown(void)
{
	shader_free(rndr.sprite_pipeline.shader);

	if (rndr.ctx) {
		SDL_GL_DeleteContext(rndr.ctx);
	}
	if (rndr.win) {
		SDL_DestroyWindow(rndr.win);
	}
}

void
renderer_clear(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void
renderer_present(void)
{
	assert(rndr.initialized);
	SDL_GL_SwapWindow(rndr.win);
}

struct RenderList*
render_list_new(void)
{
	assert(rndr.initialized);
	struct RenderList *list = make(struct RenderList);
	return list;
}

void
render_list_destroy(struct RenderList *list)
{
	destroy(list);
}

void
render_list_add_sprite(
	struct RenderList *list,
	const struct Sprite *spr,
	float x,
	float y,
	float angle
) {
	assert(list->len < RENDER_LIST_MAX_LEN);

	// initialize sprite render node
	struct RenderNode *node = &list->nodes[list->len++];
	node->type = RENDER_NODE_SPRITE;
	node->sprite = (struct Sprite*)spr;

	// compute transform
	Mat t, r;
	mat_ident(&t);
	mat_translate(&t, x, -y, 0);

	mat_ident(&r);
	mat_translate(&r, -spr->width / 2, spr->height / 2, 0);
	mat_rotate(&r, 0, 0, 1, angle);

	mat_mul(&t, &r, &node->transform);
}

static int
render_sprite_node(const struct RenderNode *node)
{
	int ok = 1;

	// configure size
	Vec size = {{ node->sprite->width, node->sprite->height, 0, 0 }};
	ok &= shader_uniform_set(
		&rndr.sprite_pipeline.u_size,
		1,
		&size
	);

	// configure transform
	Mat mvp;
	mat_mul(&rndr.projection, &node->transform, &mvp);
	ok &= shader_uniform_set(
		&rndr.sprite_pipeline.u_transform,
		1,
		&mvp
	);

	// configure texture sampler
	GLuint texture_unit = SPRITE_TEXTURE_UNIT;
	ok &= shader_uniform_set(
		&rndr.sprite_pipeline.u_texture,
		1,
		&texture_unit
	);

	// render
	glActiveTexture(GL_TEXTURE0 + texture_unit);
	glBindTexture(GL_TEXTURE_RECTANGLE, node->sprite->texture->hnd);
	glBindVertexArray(node->sprite->vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	ok &= glGetError() == GL_NO_ERROR;

	return ok;
}

void
render_list_add_text(
	struct RenderList *list,
	const struct Text *txt,
	float x,
	float y
) {
	// initialize text render node
	struct RenderNode *node = &list->nodes[list->len++];
	node->type = RENDER_NODE_TEXT;
	node->text = (struct Text*)txt;
	mat_ident(&node->transform);
	mat_translate(&node->transform, x, -y, 0);
}

static int
render_text_node(const struct RenderNode *node)
{
	int ok = 1;

	// configure transform
	Mat mvp;
	mat_mul(&rndr.projection, &node->transform, &mvp);
	ok &= shader_uniform_set(
		&rndr.text_pipeline.u_transform,
		1,
		&mvp
	);

	// configure atlas offset
	unsigned int offset = font_get_atlas_offset(node->text->font);
	ok &= shader_uniform_set(
		&rndr.text_pipeline.u_atlas_offset,
		1,
		&offset
	);

	// configure texture samplers
	GLuint glyph_texture_unit = TEXT_GLYPH_TEXTURE_UNIT;
	GLuint atlas_texture_unit = TEXT_ATLAS_TEXTURE_UNIT;
	ok &= shader_uniform_set(
		&rndr.text_pipeline.u_glyph_texture,
		1,
		&glyph_texture_unit
	);
	ok &= shader_uniform_set(
		&rndr.text_pipeline.u_atlas_texture,
		1,
		&atlas_texture_unit
	);

	// render
	glActiveTexture(GL_TEXTURE0 + atlas_texture_unit);
	glBindTexture(
		GL_TEXTURE_RECTANGLE,
		font_get_atlas_texture(node->text->font)
	);
	glActiveTexture(GL_TEXTURE0 + glyph_texture_unit);
	glBindTexture(
		GL_TEXTURE_1D,
		font_get_glyph_texture(node->text->font)
	);
	glBindVertexArray(node->text->vao);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, node->text->len);

	ok &= glGetError() == GL_NO_ERROR;

	return ok;
}

static int
node_cmp(const void *a, const void *b)
{
	if (((struct RenderNode*)a)->type == ((struct RenderNode*)b)->type) {
		return 0;
	} else if (a < b) {
		return -1;
	}
	return 1;
}

int
render_list_exec(struct RenderList *list)
{
	int ok = 1;

	// sort the list by node type
	qsort(list->nodes, list->len, sizeof(struct RenderNode), node_cmp);

	int active = -1;
	for (size_t i = 0; i < list->len; i++) {
		struct RenderNode *node = &list->nodes[i];
		switch (node->type) {
		case RENDER_NODE_SPRITE:
			if (active != node->type) {
				ok &= shader_bind(rndr.sprite_pipeline.shader);
			}
			ok &= render_sprite_node(node);
			break;
		case RENDER_NODE_TEXT:
			if (active != node->type) {
				ok &= shader_bind(rndr.text_pipeline.shader);
			}
			ok &= render_text_node(node);
			break;
		}
		active = node->type;

		if (!ok) {
			break;
		}
	}
	list->len = 0;

	return ok;
}