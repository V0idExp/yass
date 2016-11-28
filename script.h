#pragma once

#include "game.h"

struct ScriptEnv {
	struct lua_State *state;
};

struct ScriptEnv*
script_env_new(void);

int
script_env_init(struct ScriptEnv *env, const struct World *world);

void
script_env_destroy(struct ScriptEnv *env);