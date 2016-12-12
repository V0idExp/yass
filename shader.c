#include "ioutils.h"
#include "shader.h"
#include "strutils.h"
#include "memory.h"
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static size_t
compute_uniform_size(struct ShaderUniform *uniform)
{
	switch (uniform->type) {
	case GL_INT:
	case GL_SAMPLER_2D:
	case GL_SAMPLER_2D_RECT:
	case GL_SAMPLER_1D:
	case GL_INT_SAMPLER_1D:
	case GL_UNSIGNED_INT_SAMPLER_1D:
		return uniform->count * sizeof(GLint);
	case GL_BOOL:
		return uniform->count * sizeof(GLboolean);
	case GL_UNSIGNED_INT:
		return uniform->count * sizeof(GLuint);
	case GL_FLOAT:
		return uniform->count * sizeof(GLfloat);
	case GL_FLOAT_MAT4:
		return uniform->count * sizeof(GLfloat) * 16;
	case GL_FLOAT_VEC4:
		return uniform->count * sizeof(GLfloat) * 4;
	case GL_UNSIGNED_INT_VEC4:
		return uniform->count * sizeof(GLuint) * 4;
	case GL_FLOAT_VEC3:
		return uniform->count * sizeof(GLfloat) * 3;
	case GL_FLOAT_VEC2:
		return uniform->count * sizeof(GLfloat) * 2;
	}
	fprintf(stderr,
		"cannot compute the size of unknown uniform type %d\n",
		uniform->type
	);
	return 0;  // unknown uniform type
}

struct ShaderSource*
shader_source_from_string(const char *source, GLenum type)
{
	GLenum gl_err = GL_NO_ERROR;

	struct ShaderSource *ss = malloc(sizeof(struct ShaderSource));
	if (!ss) {
		return NULL;
	}
	memset(ss, 0, sizeof(struct ShaderSource));

	// create the shader
	GLuint shader = glCreateShader(type);
	if (!shader) {
		fprintf(stderr, "failed to create shader (OpenGL error %d)\n", gl_err);
		goto error;
	}

	// set shader and compile it
	glShaderSource(shader, 1, (const char**)&source, NULL);
	glCompileShader(shader);

	int status = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE) {
		// fetch compile log
		int log_len;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);
		char log[log_len];
		glGetShaderInfoLog(shader, log_len, NULL, log);

		fprintf(stderr, "shader compile error: %s\n", log);
		goto error;
	}

	ss->src = shader;

	return ss;

error:
	shader_source_free(ss);
	return NULL;
}

struct ShaderSource*
shader_source_from_file(const char *filename)
{
	const char *ext = strrchr(filename, '.');
	GLenum type;
	if (strncmp(ext, ".vert", 4) == 0) {
		type = GL_VERTEX_SHADER;
	} else if (strncmp(ext, ".frag", 4) == 0) {
		type = GL_FRAGMENT_SHADER;
	} else {
		fprintf(stderr,
			"bad shader source filename '%s'; "
			"extension must be .vert or .frag\n",
			filename
		);
		return NULL;
	}

	char *source = NULL;
	if (!file_read(filename, &source)) {
		return NULL;
	}

	struct ShaderSource *src = shader_source_from_string(source, type);
	if (!src) {
		fprintf(stderr, "shader source '%s' compilation failed\n", filename);
	}

	free(source);

	return src;
}

void
shader_source_free(struct ShaderSource *src)
{
	if (src) {
		glDeleteShader(src->src);
		free(src);
	}
}

static int
query_uniform_block(
	struct Shader *shader,
	GLuint index,
	size_t max_name_len
) {
	struct ShaderUniformBlock *block = &shader->blocks[index];

	// query uniform block name
	char name[max_name_len];
	glGetActiveUniformBlockName(
		shader->prog,
		index,
		max_name_len,
		NULL,
		name
	);
	if (!(block->name = string_copy(name))) {
		return 0;
	}

	// query block index
	block->index = glGetUniformBlockIndex(shader->prog, name);
	if (block->index == GL_INVALID_INDEX) {
		fprintf(stderr, "got invalid uniform block index\n");
		return 0;
	}

	// query block size
	glGetActiveUniformBlockiv(
		shader->prog,
		index,
		GL_UNIFORM_BLOCK_DATA_SIZE,
		(GLint*)&block->size
	);

	// query the number of active uniforms within the block and allocate
	// space for their indices and query them
	glGetActiveUniformBlockiv(
		shader->prog,
		index,
		GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS,
		(GLint*)&block->uniform_count
	);
	GLuint uniform_indices[block->uniform_count];
	glGetActiveUniformBlockiv(
		shader->prog,
		index,
		GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES,
		(GLint*)uniform_indices
	);

	// allocate block uniforms array
	size_t uniforms_size = sizeof(struct ShaderUniform) * block->uniform_count;
	if (!(block->uniforms = malloc(uniforms_size))) {
		return 0;
	}
	memset(block->uniforms, 0, uniforms_size);

	// query uniform sizes
	GLint uniform_sizes[block->uniform_count];
	glGetActiveUniformsiv(
		shader->prog,
		block->uniform_count,
		uniform_indices,
		GL_UNIFORM_SIZE,
		uniform_sizes
	);

	// query uniform types
	GLint uniform_types[block->uniform_count];
	glGetActiveUniformsiv(
		shader->prog,
		block->uniform_count,
		uniform_indices,
		GL_UNIFORM_TYPE,
		uniform_types
	);

	// query uniform offsets within the block
	GLint uniform_offsets[block->uniform_count];
	glGetActiveUniformsiv(
		shader->prog,
		block->uniform_count,
		uniform_indices,
		GL_UNIFORM_OFFSET,
		uniform_offsets
	);

	// query uniform name lengths
	GLint uniform_name_lengths[block->uniform_count];
	glGetActiveUniformsiv(
		shader->prog,
		block->uniform_count,
		uniform_indices,
		GL_UNIFORM_NAME_LENGTH,
		uniform_name_lengths
	);

	// query the information about each block uniform
	for (size_t i = 0; i < block->uniform_count; i++) {
		struct ShaderUniform *uniform = &block->uniforms[i];
		uniform->count = uniform_sizes[i];
		uniform->type = uniform_types[i];
		uniform->offset = uniform_offsets[i];
		uniform->loc = -1;
		if (!(uniform->size = compute_uniform_size(uniform))) {
			return 0;
		}

		// retrieve uniform name
		uniform->name = malloc(uniform_name_lengths[i]);
		glGetActiveUniformName(
			shader->prog,
			uniform_indices[i],
			uniform_name_lengths[i],
			NULL,
			(GLchar*)uniform->name
		);
	}

	return 1;
}

static int
init_shader_uniform_blocks(struct Shader *s)
{
	// query the number of uniform blocks
	glGetProgramiv(s->prog, GL_ACTIVE_UNIFORM_BLOCKS, (GLint*)&s->block_count);
	if (s->block_count == 0) {
		// it's ok to have no uniform blocks
		return 1;
	}

	// allocate uniform blocks array
	size_t blocks_size = sizeof(struct ShaderUniformBlock) * s->block_count;
	s->blocks = malloc(blocks_size);
	if (!s->blocks) {
		return 0;
	}
	memset(s->blocks, 0, blocks_size);

	// query maximum block name length
	size_t max_name_len = 0;
	glGetProgramiv(
		s->prog,
		GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH,
		(GLint*)&max_name_len
	);

	// populate blocks array
	for (size_t i = 0; i < s->block_count; i++) {
		if (!query_uniform_block(s, i, max_name_len)) {
			fprintf(stderr, "failed to query uniform block information");
			return 0;
		}
	}
	return 1;
}

static int
init_shader_uniforms(struct Shader *s)
{
	// query the number of uniforms in shader proram
	GLuint count = 0;
	glGetProgramiv(s->prog, GL_ACTIVE_UNIFORMS, (GLint*)&count);
	if (count == 0) {
		s->uniform_count = 0;
		s->uniforms = NULL;
		return 1;
	}

	// prepare name string buffer
	GLsizei name_len;
	glGetProgramiv(s->prog, GL_ACTIVE_UNIFORM_MAX_LENGTH, (GLint*)&name_len);
	GLchar name[name_len];

	// query information about each uniform
	size_t uniform_sizes[count];
	GLenum uniform_types[count];
	GLint uniform_locations[count];
	char* uniform_names[count];
	size_t actual_count = 0;
	for (size_t i = 0; i < count; i++) {
		// query the size, type and name information
		glGetActiveUniform(
			s->prog,
			i,
			name_len,
			NULL,
			(GLint*)&uniform_sizes[actual_count],
			&uniform_types[actual_count],
			(GLchar*)name
		);

		// only uniforms which have a location different from -1 are to
		// be stored, others belong to uniform blocks
		GLint loc = glGetUniformLocation(s->prog, (GLchar*)name);
		if (loc != -1) {
			uniform_names[actual_count] = string_copy(name);
			uniform_locations[actual_count] = loc;
			actual_count++;
		}
	}

	// fill uniforms array
	if (actual_count > 0) {
		size_t uniforms_size = sizeof(struct ShaderUniform) * actual_count;
		s->uniforms = malloc(uniforms_size);
		if (!s->uniforms) {
			return 0;
		}
		memset(s->uniforms, 0, uniforms_size);
		s->uniform_count = actual_count;
		for (size_t i = 0; i < actual_count; i++) {
			struct ShaderUniform *uniform = &s->uniforms[i];
			uniform->name = uniform_names[i];
			uniform->loc = uniform_locations[i];
			uniform->type = uniform_types[i];
			uniform->count = uniform_sizes[i];
			uniform->offset = -1;
			if (!(uniform->size = compute_uniform_size(uniform))) {
				return 0;
			}
		}
	}

	return 1;
}

struct Shader*
shader_new(struct ShaderSource **sources, unsigned count)
{
	assert(sources != NULL);
	assert(count > 0);

	GLuint prog = 0;
	struct Shader *shader = NULL;

	// create shader program
	prog = glCreateProgram();
	if (!prog) {
		fprintf(stderr,
			"failed to create shader program (OpenGL error %d)\n",
			glGetError()
		);
		goto error;
	}

	// attach shaders and link the program
	for (unsigned i = 0; i < count; i++) {
		assert(sources[i]->src != 0);
		glAttachShader(prog, sources[i]->src);
	}
	glLinkProgram(prog);

	// retrieve link status
	int status = GL_FALSE;
	glGetProgramiv(prog, GL_LINK_STATUS, &status);
	if (status == GL_FALSE) {
		// retrieve link log
		int log_len;
		glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &log_len);
		char log[log_len];
		glGetProgramInfoLog(prog, log_len, NULL, log);

		fprintf(stderr, "failed to link shader program: %s \n", log);
		goto error;
	}

	shader = make(struct Shader);
	shader->prog = prog;
	if (!init_shader_uniform_blocks(shader) ||
	    !init_shader_uniforms(shader)) {
		fprintf(stderr, "failed to initialize shader uniforms table");
		goto error;
	}

	return shader;

error:
	shader_free(shader);
	return NULL;
}

struct Shader*
shader_compile(
	const char *vert_src_filename,
	const char *frag_src_filename,
	const char *uniform_names[],
	struct ShaderUniform *r_uniforms[],
	const char *uniform_block_names[],
	struct ShaderUniformBlock *r_uniform_blocks[]
) {
	// compile sources and attempt to create a shader program
	struct Shader *shader = NULL;
	struct ShaderSource *sources[2] = { NULL, NULL };
	sources[0] = shader_source_from_file(vert_src_filename);
	sources[1] = shader_source_from_file(frag_src_filename);
	if (!sources[0] || !sources[1] || !(shader = shader_new(sources, 2))) {
		goto error;
	}

	int ok = 1;

	// lookup uniforms
	if (uniform_names) {
		assert(r_uniforms != NULL);
		ok &= shader_get_uniforms(shader, uniform_names, r_uniforms);
	}

	// lookup uniform blocks
	if (uniform_block_names) {
		assert(r_uniform_blocks != NULL);
		ok &= shader_get_uniform_blocks(
			shader,
			uniform_block_names,
			r_uniform_blocks
		);
	}

	if (!ok) {
		goto error;
	}

	// TODO: return the compiled shader sources

	return shader;

error:
	for (unsigned i = 0; i < sizeof(sources) / sizeof(struct ShaderSource*); i++) {
		shader_source_free(sources[i]);
	}
	shader_free(shader);
	return NULL;
}

void
shader_free(struct Shader *s)
{
	if (s) {
		for (size_t i = 0; i < s->uniform_count; i++) {
			free((char*)s->uniforms[i].name);
		}
		free(s->uniforms);

		for (size_t i = 0; i < s->block_count; i++) {
			struct ShaderUniformBlock *block = &s->blocks[i];
			for (size_t j = 0; j < block->uniform_count; j++) {
				free((char*)block->uniforms[j].name);
			}
			free(block->uniforms);
		}
		free(s->blocks);
		free(s);
	}
}

int
shader_bind(struct Shader *s)
{
	assert(s != NULL);

	glUseProgram(s->prog);

#ifdef DEBUG
	GLenum gl_err;
	if ((gl_err = glGetError()) != GL_NO_ERROR) {
		fprintf(stderr,
			"failed to bind shader %d (OpenGL error %d)\n",
			s->prog,
			gl_err
		);
		return 0;
	}
#endif

	return 1;
}

const struct ShaderUniform*
shader_get_uniform(struct Shader *s, const char *name)
{
	assert(s != NULL);
	assert(name != NULL);

	for (GLuint i = 0; i < s->uniform_count; i++) {
		struct ShaderUniform *uniform = s->uniforms + i;
		if (strcmp(uniform->name, name) == 0) {
			return uniform;
		}
	}
	fprintf(stderr, "no such shader uniform '%s'\n", name);
	return NULL;
}

int
shader_get_uniforms(
	struct Shader *shader,
	const char *names[],
	struct ShaderUniform *r_uniforms[]
) {
	for (size_t i = 0; names[i] != NULL; i++) {
		const struct ShaderUniform *u = shader_get_uniform(
			shader,
			names[i]
		);
		if (!u) {
			return 0;
		}
		*r_uniforms[i] = *u;
	}
	return 1;
}

const struct ShaderUniformBlock*
shader_get_uniform_block(struct Shader *s, const char *name)
{
	assert(s != NULL);
	assert(name != NULL);

	for (GLuint i = 0; i < s->block_count; i++) {
		struct ShaderUniformBlock *block = &s->blocks[i];
		if (strcmp(block->name, name) == 0) {
			return block;
		}
	}
	fprintf(stderr, "no such shader uniform block '%s'\n", name);
	return NULL;
}

int
shader_get_uniform_blocks(
	struct Shader *shader,
	const char *names[],
	struct ShaderUniformBlock *r_uniform_blocks[]
) {
	for (size_t i = 0; names[i] != NULL; i++) {
		const struct ShaderUniformBlock *ub = shader_get_uniform_block(
			shader,
			names[i]
		);
		if (!ub) {
			return 0;
		}
		*r_uniform_blocks[i] = *ub;
	}
	return 1;
}

const struct ShaderUniform*
shader_uniform_block_get_uniform(const struct ShaderUniformBlock *block, const char *name)
{
	assert(block != NULL);
	assert(name != NULL);

	for (size_t i = 0; i < block->uniform_count; i++) {
		if (strcmp(block->uniforms[i].name, name) == 0) {
			return &block->uniforms[i];
		}
	}
	fprintf(stderr, "no such uniform `%s` in uniform block `%s`\n", name, block->name);
	return NULL;
}

int
shader_uniform_set(const struct ShaderUniform *uniform, size_t count, ...)
{
	assert(uniform->loc != -1);
	assert(uniform->count <= count);

	va_list ap;
	va_start(ap, count);

	switch (uniform->type) {
	case GL_INT:
	case GL_BOOL:
	case GL_SAMPLER_2D:
	case GL_SAMPLER_2D_RECT:
	case GL_SAMPLER_1D:
	case GL_INT_SAMPLER_1D:
	case GL_UNSIGNED_INT_SAMPLER_1D:
		glUniform1iv(uniform->loc, count, va_arg(ap, GLint*));
		break;

	case GL_UNSIGNED_INT:
		glUniform1uiv(uniform->loc, count, va_arg(ap, GLuint*));
		break;

	case GL_FLOAT:
		glUniform1fv(uniform->loc, count, va_arg(ap, GLfloat*));
		break;

	case GL_FLOAT_MAT4:
		glUniformMatrix4fv(uniform->loc, count, GL_TRUE, va_arg(ap, GLfloat*));
		break;

	case GL_FLOAT_VEC4:
		glUniform4fv(uniform->loc, count, va_arg(ap, GLfloat*));
		break;

	case GL_UNSIGNED_INT_VEC4:
		{
			Vec fv = *va_arg(ap, Vec*);
			GLuint uiv[4] = { fv.data[0], fv.data[1], fv.data[2], fv.data[3] };
			glUniform4uiv(uniform->loc, 1, uiv);
		}
		break;
	case GL_FLOAT_VEC3:
		{
			GLfloat data[count][3];
			Vec *v = va_arg(ap, Vec*);
			for (size_t i = 0; i < count; i++) {
				memcpy(data[i], v + i, sizeof(GLfloat) * 3);
			}
			glUniform3fv(uniform->loc, count, (GLfloat*)data);
		}
		break;
	case GL_FLOAT_VEC2:
		{
			GLfloat data[count][2];
			Vec *v = va_arg(ap, Vec*);
			for (size_t i = 0; i < count; i++) {
				memcpy(data[i], v + i, sizeof(GLfloat) * 2);
			}
			glUniform2fv(uniform->loc, count, (GLfloat*)data);
		}
		break;
	}
	va_end(ap);

#ifdef DEBUG
	GLenum gl_errno = glGetError();
	if (gl_errno != GL_NO_ERROR) {
		fprintf(stderr,
			"failed to set shader uniform '%s' (OpenGL error %d)\n",
			uniform->name,
			gl_errno
		);
		return 0;
	}
#endif
	return 1;
}
