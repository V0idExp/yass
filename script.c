#include <assert.h>
#include <stdio.h>
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "memory.h"
#include "script.h"


struct ScriptEnv*
script_env_new(void)
{
	struct ScriptEnv *env = make(struct ScriptEnv);
	if (!env) {
		return NULL;
	}

	env->state = luaL_newstate();
	if (!env->state) {
		script_env_destroy(env);
		return NULL;
	}

	luaL_openlibs(env->state);

	// retrieve version and print it
	lua_getglobal(env->state, "_VERSION");
	printf("Initialized %s environment\n", lua_tostring(env->state, -1));
	lua_pop(env->state, 1);

	return env;
}

int
script_env_init(struct ScriptEnv *env, const struct World *world)
{
	assert(env);
	assert(world);

	return 1;
}

void
script_env_destroy(struct ScriptEnv *env)
{
	if (env) {
		if (env->state) {
			lua_close(env->state);
		}
		destroy(env);
	}
}