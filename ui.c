#include "font.h"
#include "game.h"
#include "layout.h"
#include "matlib.h"
#include "renderer.h"
#include "state.h"
#include "text.h"
#include "texture.h"
#include "ui.h"

/*** UI STATE ***/
static struct {
	int show_upgrades_win;
} ui_state = {
	0
};

/*** RESOURCES ***/
static struct Font *font_dbg = NULL;
static struct Font *font_ui_l = NULL;
static struct Font *font_ui_m = NULL;
static struct Font *font_ui_s = NULL;


static struct Text *text_fps = NULL;
static struct Text *text_render_time = NULL;
static struct Text *text_credits = NULL;
static struct Text *text_upgrade_weapon_btn_label = NULL;
static struct Text *text_upgrade_weapon_cost = NULL;
static struct Text *text_upgrade_weapon_label = NULL;

static struct Texture *tex_block_green = NULL;
static struct Texture *tex_block_red = NULL;
static struct Texture *tex_block_white = NULL;
static struct Texture *tex_block_shadow = NULL;
static struct Texture *tex_win = NULL;
static struct Texture *tex_frame = NULL;
static struct Texture *tex_btn = NULL;

// TEXTURES
static const struct TextureRes {
	const char *file;
	struct Texture **var;
	struct Border border;
} textures[] = {
	{
		"data/art/UI/squareGreen.png",
		&tex_block_green,
		{ 6, 6 }
	},
	{
		"data/art/UI/squareRed.png",
		&tex_block_red,
		{ 6, 6 }
	},
	{
		"data/art/UI/squareWhite.png",
		&tex_block_white,
		{ 6, 6 }
	},
	{
		"data/art/UI/square_shadow.png",
		&tex_block_shadow,
		{ 6, 6 }
	},
	{
		"data/art/UI/metalPanel_red.png",
		&tex_win,
		{ 11, 11, 32, 32 }
	},
	{
		"data/art/UI/metalPanel_plate.png",
		&tex_frame,
		{ 7, 7, 7, 7 }
	},
	{
		"data/art/UI/buttonRed.png",
		&tex_btn,
		{ 6, 6, 6, 6 }
	},
	{ NULL }
};

// FONTS
static const struct {
	const char *file;
	unsigned size;
	struct Font **var;
} fonts[] = {
	{ "data/fonts/courier.ttf", 16, &font_dbg },
	{ "data/fonts/kenvector_future_thin.ttf", 19, &font_ui_l },
	{ "data/fonts/kenvector_future_thin.ttf", 16, &font_ui_m },
	{ "data/fonts/kenvector_future_thin.ttf", 14, &font_ui_s },
	{ NULL }
};

// WIDGETS
enum {
	WIDGET_CONTAINER,
	WIDGET_TEXT,
	WIDGET_IMAGE,
};

struct Widget {
	int type;
	struct Element *elem;
	float z;
	int visible;
	int enabled;
	float opacity;
	int (*on_click)(EventDispatchFunc dispatch, void *context);
	union {
		struct {
			struct Text *text;
			Vec color;
		} text;
		struct Texture *image;
	};
};

static struct Widget w_root;
static struct Widget w_hp_bar;
static struct Widget w_hp_bar_bg;
static struct Widget w_credits;
static struct Widget w_fps;
static struct Widget w_render_time;
static struct Widget w_upgrades_win;
static struct Widget w_upgrades_weapon_frame;
static struct Widget w_upgrades_weapon_btn;
static struct Widget w_upgrades_weapon_btn_label;
static struct Widget w_upgrades_weapon_cost_label;
static struct Widget w_upgrades_weapon_label;
static struct Widget w_upgrades_weapon_level[6];

// WIDGET EVENT HANDLERS
static int
on_upgrade_weapon_btn_click(EventDispatchFunc dispatch, void *context);

// UI LAYOUT SPEC
static struct WidgetSpec {
	struct Widget *var, *parent;
	int type;
	struct Anchors anchors;
	struct Margins margins;
	Measure width, height;
	int x, y, z;
	int (*on_click)(EventDispatchFunc dispatch, void *context);
	union {
		struct {
			struct Text **text;
			Vec color;
		} text;
		struct Texture **image;
	};
} *layout;

static int
init_layout(void)
{
	struct WidgetSpec layout_template[] = {
		// root element (screen)
		{
			.var = &w_root,
			.parent = NULL,
			.type = WIDGET_CONTAINER,
			.width = measure_px(SCREEN_WIDTH),
			.height = measure_px(SCREEN_HEIGHT)
		},
		// hitpoints bar
		{
			.var = &w_hp_bar,
			.parent = &w_root,
			.type = WIDGET_IMAGE,
			.image = &tex_block_green,
			.anchors = {
				.left = ANCHOR_LEFT,
				.top = ANCHOR_TOP
			},
			.margins = {
				.left = measure_px(20),
				.top = measure_px(20)
			},
			.width = measure_px(200),
			.height = measure_px(26),
			.z = 1.0
		},
		// hitpoints bar background
		{
			.var = &w_hp_bar_bg,
			.parent = &w_hp_bar,
			.type = WIDGET_IMAGE,
			.image = &tex_block_red,
			.anchors = {
				.left = ANCHOR_LEFT,
				.top = ANCHOR_TOP,
			},
			.width = measure_px(200),
			.height = measure_px(26)
		},
		// FPS text
		{
			.var = &w_fps,
			.parent = &w_hp_bar,
			.type = WIDGET_TEXT,
			.text = {
				.text = &text_fps,
				.color = {{1.0, 1.0, 1.0, 1.0}}
			},
			.anchors = {
				.top = ANCHOR_BOTTOM,
				.left = ANCHOR_LEFT,
			},
			.margins = {
				.top = measure_px(10)
			}
		},
		// render time text
		{
			.var = &w_render_time,
			.parent = &w_fps,
			.type = WIDGET_TEXT,
			.text = {
				.text = &text_render_time,
				.color = {{1.0, 1.0, 1.0, 1.0}}
			},
			.anchors = {
				.top = ANCHOR_BOTTOM,
				.left = ANCHOR_LEFT,
			},
			.margins = {
				.top = measure_px(10)
			},
		},
		// credits text
		{
			.var = &w_credits,
			.parent = &w_root,
			.type = WIDGET_TEXT,
			.text = {
				.text = &text_credits,
				.color = {{1.0, 1.0, 1.0, 1.0}}
			},
			.anchors = {
				.top = ANCHOR_TOP,
				.right = ANCHOR_RIGHT
			},
			.margins = {
				.top = measure_px(27)
			},
			.width = measure_px(200)
		},
		// upgrades window
		{
			.var = &w_upgrades_win,
			.parent = &w_root,
			.type = WIDGET_IMAGE,
			.image = &tex_win,
			.anchors = {
				.hcenter = ANCHOR_HCENTER,
				.vcenter = ANCHOR_VCENTER
			},
			.width = measure_px(450),
			.height = measure_px(450),
		},
		// upgrades window - weapons frame
		{
			.var = &w_upgrades_weapon_frame,
			.parent = &w_upgrades_win,
			.type = WIDGET_IMAGE,
			.image = &tex_frame,
			.anchors = {
				.left = ANCHOR_LEFT,
				.right = ANCHOR_RIGHT,
				.top = ANCHOR_TOP,
			},
			.margins = {
				.top = measure_px(32),
				.left = measure_px(8),
				.right = measure_px(8),
			},
			.height = measure_px(130),
			.z = 1,
		},
		// upgrades window - weapons button
		{
			.var = &w_upgrades_weapon_btn,
			.parent = &w_upgrades_weapon_frame,
			.type = WIDGET_IMAGE,
			.image = &tex_btn,
			.anchors = {
				.right = ANCHOR_RIGHT,
				.bottom = ANCHOR_BOTTOM,
			},
			.margins = {
				.right = measure_px(8),
				.bottom = measure_px(8)
			},
			.width = measure_px(150),
			.height = measure_px(39),
			.z = 2,
			.on_click = on_upgrade_weapon_btn_click
		},
		// upgrades window - weapon button label
		{
			.var = &w_upgrades_weapon_btn_label,
			.parent = &w_upgrades_weapon_btn,
			.type = WIDGET_TEXT,
			.text = {
				.text = &text_upgrade_weapon_btn_label,
				.color = {{1.0, 0.0, 0.0, 1.0}}
			},
			.anchors = {
				.hcenter = ANCHOR_HCENTER,
				.vcenter = ANCHOR_VCENTER,
			},
			.z = 3,
		},
		// upgrades window - weapon cost label
		{
			.var = &w_upgrades_weapon_cost_label,
			.parent = &w_upgrades_weapon_frame,
			.type = WIDGET_TEXT,
			.text = {
				.text = &text_upgrade_weapon_cost,
				.color = {{0.4, 0.4, 0.4, 1.0}}
			},
			.anchors = {
				.left = ANCHOR_LEFT,
				.bottom = ANCHOR_BOTTOM
			},
			.margins = {
				.left = measure_px(8),
				.bottom = measure_px(22)
			},
			.z = 3
		},
		// upgrades window - weapon label
		{
			.var = &w_upgrades_weapon_label,
			.parent = &w_upgrades_weapon_frame,
			.type = WIDGET_TEXT,
			.text = {
				.text = &text_upgrade_weapon_label,
				.color = {{0.8, 0.4, 0.4, 1.0}}
			},
			.anchors = {
				.left = ANCHOR_LEFT,
				.top = ANCHOR_TOP
			},
			.margins = {
				.left = measure_px(8),
				.top = measure_px(8)
			},
			.z = 3
		},
		// upgrades window - weapon level widgets
		{
			.var = &w_upgrades_weapon_level[0],
			.parent = &w_upgrades_weapon_frame,
			.type = WIDGET_IMAGE,
			.image = &tex_block_shadow,
			.anchors = {
				.left = ANCHOR_LEFT,
				.top = ANCHOR_TOP,
			},
			.margins = {
				.top = measure_px(35),
				.left = measure_pc(2),
			},
			.width = measure_pc(30),
			.height = measure_px(26),
			.z = 3
		},
		{
			.var = &w_upgrades_weapon_level[1],
			.parent = &w_upgrades_weapon_frame,
			.type = WIDGET_IMAGE,
			.image = &tex_block_shadow,
			.anchors = {
				.left = ANCHOR_LEFT,
				.top = ANCHOR_TOP,
			},
			.margins = {
				.top = measure_px(35),
				.left = measure_pc(35),
			},
			.width = measure_pc(30),
			.height = measure_px(26),
			.z = 3
		},
		{
			.var = &w_upgrades_weapon_level[2],
			.parent = &w_upgrades_weapon_frame,
			.type = WIDGET_IMAGE,
			.image = &tex_block_shadow,
			.anchors = {
				.left = ANCHOR_LEFT,
				.top = ANCHOR_TOP,
			},
			.margins = {
				.top = measure_px(35),
				.left = measure_pc(68),
			},
			.width = measure_pc(30),
			.height = measure_px(26),
			.z = 3
		},
		{
			.var = NULL
		}
	};
	layout = malloc(sizeof(layout_template));
	if (!layout) {
		return 0;
	}
	memcpy(layout, layout_template, sizeof(layout_template));
	return 1;
};

static void
update_credits(int credits)
{
	text_set_fmt(text_credits, "Credits: %d$", credits);
	w_credits.elem->height = measure_px(text_credits->height);
}

static void
update_hitpoints(int hitpoints)
{
	w_hp_bar.elem->width = measure_px(
		200.0 * hitpoints / PLAYER_INITIAL_HITPOINTS
	);
}

static void
update_stats(unsigned fps, unsigned render_time)
{
	text_set_fmt(text_fps, "FPS: %d", fps);
	w_fps.elem->width = measure_px(text_fps->width);
	w_fps.elem->height = measure_px(text_fps->height);

	text_set_fmt(text_render_time, "Render time: %dms", render_time);
	w_render_time.elem->width = measure_px(text_render_time->width);
	w_render_time.elem->height = measure_px(text_render_time->height);
}

static void
update_upgrades_win(int cannons_level)
{
	text_set_fmt(
		text_upgrade_weapon_label,
		"Cannons: Level %d",
		cannons_level
	);
	w_upgrades_weapon_label.elem->width = measure_px(
		text_upgrade_weapon_label->width
	);
	w_upgrades_weapon_label.elem->height = measure_px(
		text_upgrade_weapon_label->height
	);

	for (short i = 0; i < 3; i++) {
		if (i + 1 <= cannons_level) {
			w_upgrades_weapon_level[i].image = tex_block_white;
		}
	}
}

int
ui_load(void)
{
	// load textures
	for (unsigned i = 0; textures[i].file != NULL; i++) {
		struct TextureRes res = textures[i];
		if (!(*res.var = texture_from_file(res.file))) {
			fprintf(
				stderr,
				"failed to load texture `%s`\n",
				res.file
			);
			return 0;
		}
		(*res.var)->border = res.border;
		printf("loaded texure `%s`\n", res.file);
	}

	// load fonts
	for (unsigned i = 0; fonts[i].file != NULL; i++) {
		if (!(*fonts[i].var = font_from_file(fonts[i].file, fonts[i].size))) {
			fprintf(
				stderr,
				"failed to load font `%s` (%d)\n",
				fonts[i].file,
				fonts[i].size
			);
			return 0;
		}
		printf("loaded font `%s` (%d)\n", fonts[i].file, fonts[i].size);
	}

	// create text renderables
	text_fps = text_new(font_dbg);
	text_render_time = text_new(font_dbg);
	text_credits = text_new(font_ui_l);
	text_upgrade_weapon_btn_label = text_new(font_ui_s);
	text_upgrade_weapon_cost = text_new(font_ui_s);
	text_upgrade_weapon_label = text_new(font_ui_l);
	if (!text_fps ||
	    !text_render_time ||
	    !text_credits ||
	    !text_upgrade_weapon_btn_label ||
	    !text_upgrade_weapon_cost ||
	    !text_upgrade_weapon_label) {
		return 0;
	}

	// initialize layout
	if (!init_layout()) {
		return 0;
	}
	for (size_t i = 0; layout[i].var != NULL; i++) {
		const struct WidgetSpec spec = layout[i];
		struct Widget *widget = spec.var;

		// create a layout element for the widget
		struct Element *elem = element_new(spec.width, spec.height);
		if (!elem) {
			return 0;
		}

		// add it to parent element, if specified
		if (spec.parent &&
		    !element_add_child(spec.parent->elem, elem)) {
			return 0;
		}

		// initialize element data from widget spec
		elem->anchors = spec.anchors;
		elem->margins = spec.margins;
		elem->x = spec.x;
		elem->y = spec.y;
		elem->width = spec.width;
		elem->height = spec.height;
		elem->userdata = widget;

		// initialize widget
		widget->visible = 1;
		widget->type = spec.type;
		widget->elem = elem;
		widget->z = spec.z;
		widget->on_click = spec.on_click;
		widget->opacity = 1.0;
		widget->enabled = 1;

		switch (widget->type) {
		case WIDGET_TEXT:
			widget->text.text = *spec.text.text;
			widget->text.color = spec.text.color;
			break;
		case WIDGET_IMAGE:
			widget->image = *spec.image;
			break;
		}
	}

	update_credits(0);
	update_hitpoints(0);
	update_upgrades_win(0);

	text_set_string(text_upgrade_weapon_btn_label, "Buy upgrade");
	w_upgrades_weapon_btn_label.elem->width = measure_px(
		text_upgrade_weapon_btn_label->width
	);
	w_upgrades_weapon_btn_label.elem->height = measure_px(
		text_upgrade_weapon_btn_label->height
	);

	text_set_fmt(
		text_upgrade_weapon_cost,
		"Cost: %d$",
		WEAPON_UPGRADE_COST
	);
	w_upgrades_weapon_cost_label.elem->width = measure_px(
		text_upgrade_weapon_cost->width
	);
	w_upgrades_weapon_cost_label.elem->height = measure_px(
		text_upgrade_weapon_cost->height
	);

	printf("UI initialized\n");

	return 1;
}

void
ui_cleanup(void)
{
	free(layout);

	// destroy text objects
	text_destroy(text_fps);
	text_destroy(text_render_time);
	text_destroy(text_credits);
	text_destroy(text_upgrade_weapon_btn_label);
	text_destroy(text_upgrade_weapon_cost);

	// destroy all layout trees
	for (unsigned i = 0; layout[i].var; i++) {
		struct Widget *w = layout[i].var;
		if (w->elem && w->elem->parent == NULL) {
			element_destroy(w->elem);
		}
	}

	// destroy fonts
	for (unsigned i = 0; fonts[i].file; i++) {
		font_destroy(*fonts[i].var);
	}

	// destroy textures
	for (unsigned i = 0; textures[i].file; i++) {
		texture_destroy(*textures[i].var);
	}
}

int
ui_update(const struct State *state, float dt)
{
	// previous state, needed to compute diff between updates in order to
	// show changes only
	static struct State prev_state;
	static int first_update = 1;
	if (first_update) {
		memset(&prev_state, 0, sizeof(struct State));
		first_update = 0;
	}

	// update performance images once per second
	static float time_acc = 0;
	time_acc += dt;
	if (time_acc >= 1.0) {
		time_acc -= 1.0;
		update_stats(state->fps, state->render_time);
	}

	// update credits text
	if (prev_state.credits != state->credits) {
		update_credits(state->credits);
	}

	// update hitpoints image
	if (prev_state.hitpoints != state->hitpoints) {
		update_hitpoints(state->hitpoints);
	}

	if (prev_state.cannons_level != state->cannons_level) {
		update_upgrades_win(state->cannons_level);
	}

	// update local UI state
	ui_state.show_upgrades_win = state->show_upgrades_shop;
	(
		w_upgrades_win.visible =
		w_upgrades_weapon_frame.visible =
		w_upgrades_weapon_btn.visible =
		w_upgrades_weapon_btn_label.visible =
		w_upgrades_weapon_cost_label.visible =
		w_upgrades_weapon_label.visible =
		w_upgrades_weapon_level[0].visible =
		w_upgrades_weapon_level[1].visible =
		w_upgrades_weapon_level[2].visible =
		ui_state.show_upgrades_win
	);

	if (ui_state.show_upgrades_win) {
		// toggle weapon upgrade button
		int enabled = state->credits >= WEAPON_UPGRADE_COST;
		float opacity = enabled ? 1.0 : 0.5;
		(
			w_upgrades_weapon_btn.enabled =
			w_upgrades_weapon_btn_label.enabled =
			enabled
		);
		(
			w_upgrades_weapon_btn.opacity =
			w_upgrades_weapon_btn_label.opacity =
			opacity
		);
	}

	// compute the new layout
	if (!element_compute_layout(w_root.elem)) {
		return 0;
	}

	prev_state = *state;

	return 1;
}

int
ui_render(struct RenderList *rndr_list)
{
	for (unsigned i = 0; layout[i].var; i++) {
		struct Widget *w = layout[i].var;
		if (!w->visible) {
			continue;
		}

		switch (w->type) {
		case WIDGET_TEXT:
			render_list_add_text(
				rndr_list,
				w->text.text,
				w->elem->x,
				w->elem->y,
				w->z,
				w->text.color,
				w->opacity
			);
			break;
		case WIDGET_IMAGE:
			render_list_add_image(
				rndr_list,
				w->image,
				w->elem->x,
				w->elem->y,
				w->z,
				w->elem->width.computed,
				w->elem->height.computed,
				w->opacity
			);
			break;
		}
	}
	return 1;
}

struct ClickContext {
	int x, y;
	int ok;
	EventDispatchFunc dispatch;
	void *dispatch_context;
};

static int
dispatch_click(struct Element *elem, void *userdata)
{
	struct ClickContext *ctx = userdata;
	struct Widget *widget = elem->userdata;
	int x = ctx->x, y = ctx->y;
	int x1 = elem->x, x2 = elem->x + elem->width.computed;
	int y1 = elem->y, y2 = elem->y + elem->height.computed;
	if (widget->enabled &&
	    widget->visible &&
	    widget->on_click &&
	    x >= x1 && x <= x2 && y >= y1 && y <= y2) {
		return (ctx->ok = widget->on_click(
			ctx->dispatch,
			ctx->dispatch_context
		));
	}
	return 1;
}

int
ui_handle_click(int x, int y, EventDispatchFunc dispatch, void *context)
{
	struct ClickContext ctx = {
		.x = x,
		.y = y,
		.ok = 1,
		.dispatch = dispatch,
		.dispatch_context = context
	};
	element_traverse(w_root.elem, dispatch_click, &ctx);
	return ctx.ok;
}

static int
on_upgrade_weapon_btn_click(EventDispatchFunc dispatch, void *context)
{
	struct Event evt = {
		.type = EVENT_CANNONS_UPGRADE
	};
	return dispatch(&evt, context);
}