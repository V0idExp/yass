#pragma once

#include "game.h"

struct ScriptEnv {
	struct lua_State *state;
	int tick_func;
};

struct ScriptEnv*
script_env_new(void);

int
script_env_init(struct ScriptEnv *env, struct World *world);

int
script_env_load_file(struct ScriptEnv *env, const char *filename);

int
script_env_tick(struct ScriptEnv *env);

void
script_env_destroy(struct ScriptEnv *env);