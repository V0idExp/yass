#pragma once

struct State;
struct RenderList;
struct Event;

typedef int (*EventDispatchFunc)(const struct Event *evt, void *context);

int
ui_load(void);

int
ui_update(const struct State *state, float dt);

int
ui_render(struct RenderList *rndr_list);

int
ui_handle_click(int x, int y, EventDispatchFunc dispatch, void *context);

void
ui_cleanup(void);