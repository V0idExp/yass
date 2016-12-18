#pragma once

struct State;
struct RenderList;

int
ui_load(void);

int
ui_update(const struct State *state, float dt);

int
ui_render(struct RenderList *rndr_list);

int
ui_handle_click(int x, int y);

void
ui_cleanup(void);