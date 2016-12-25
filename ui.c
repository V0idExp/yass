#include "font.h"
#include "game.h"
#include "layout.h"
#include "matlib.h"
#include "renderer.h"
#include "state.h"
#include "text.h"
#include "texture.h"

/*** UI STATE ***/
static struct {
	int show_upgrades_win;
} ui_state = {
	0
};

/*** RESOURCES ***/
static struct Font *font_dbg = NULL;
static struct Font *font_hud = NULL;
static struct Font *font_ui = NULL;

static struct Text *text_fps = NULL;
static struct Text *text_render_time = NULL;
static struct Text *text_credits = NULL;
static struct Text *text_upgrade_weapon = NULL;

static struct Texture *tex_hp_bar_green = NULL;
static struct Texture *tex_hp_bar_red = NULL;
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
		&tex_hp_bar_green,
		{ 6, 6 }
	},
	{
		"data/art/UI/squareRed.png",
		&tex_hp_bar_red,
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
	{ "data/fonts/kenvector_future_thin.ttf", 16, &font_hud },
	{ "data/fonts/kenvector_future_thin.ttf", 14, &font_ui },
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
	int (*on_click)(void);
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

// WIDGET EVENT HANDLERS
static int
on_upgrade_weapon_btn_click(void);

// UI LAYOUT SPEC
static const struct WidgetSpec {
	struct Widget *var, *parent;
	int type;
	struct Anchors anchors;
	struct Margins margins;
	unsigned width, height;
	int x, y, z;
	int (*on_click)(void);
	union {
		struct {
			struct Text **text;
			Vec color;
		} text;
		struct Texture **image;
	};
} layout[] = {
	// root element (screen)
	{
		.var = &w_root,
		.parent = NULL,
		.type = WIDGET_CONTAINER,
		.width = SCREEN_WIDTH,
		.height = SCREEN_HEIGHT
	},
	// hitpoints bar
	{
		.var = &w_hp_bar,
		.parent = &w_root,
		.type = WIDGET_IMAGE,
		.image = &tex_hp_bar_green,
		.anchors = {
			.left = ANCHOR_LEFT,
			.top = ANCHOR_TOP
		},
		.margins = {
			.left = 20,
			.top = 20
		},
		.width = 200,
		.height = 26,
		.z = 1.0
	},
	// hitpoints bar background
	{
		.var = &w_hp_bar_bg,
		.parent = &w_hp_bar,
		.type = WIDGET_IMAGE,
		.image = &tex_hp_bar_red,
		.anchors = {
			.left = ANCHOR_LEFT,
			.top = ANCHOR_TOP,
		},
		.width = 200,
		.height = 26
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
			.top = 10
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
			.top = 10
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
			.top = 20
		},
		.width = 150
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
		.width = 450,
		.height = 450,
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
			.top = 32,
			.left = 8,
			.right = 8,
		},
		.height = 100,
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
			.right = 8,
			.bottom = 8
		},
		.width = 150,
		.height = 39,
		.z = 2,
		.on_click = on_upgrade_weapon_btn_click
	},
	// upgrades window - weapon button label
	{
		.var = &w_upgrades_weapon_btn_label,
		.parent = &w_upgrades_weapon_btn,
		.type = WIDGET_TEXT,
		.text = {
			.text = &text_upgrade_weapon,
			.color = {{1.0, 0.0, 0.0, 1.0}}
		},
		.anchors = {
			.hcenter = ANCHOR_HCENTER,
			.vcenter = ANCHOR_VCENTER,
		},
		.z = 3,
	},
	{
		.var = NULL
	}
};

static void
update_credits(int credits)
{
	text_set_fmt(text_credits, "Credits: %d$", credits);
	w_credits.elem->height = text_credits->height;
}

static void
update_hitpoints(int hitpoints)
{
	w_hp_bar.elem->width = (200.0 * hitpoints / PLAYER_INITIAL_HITPOINTS);
}

static void
update_stats(unsigned fps, unsigned render_time)
{
	text_set_fmt(text_fps, "FPS: %d", fps);
	w_fps.elem->width = text_fps->width;
	w_fps.elem->height = text_fps->height;

	text_set_fmt(text_render_time, "Render time: %dms", render_time);
	w_render_time.elem->width = text_render_time->width;
	w_render_time.elem->height = text_render_time->height;
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
	text_credits = text_new(font_hud);
	text_upgrade_weapon = text_new(font_ui);
	if (!text_fps ||
	    !text_render_time ||
	    !text_credits ||
	    !text_upgrade_weapon) {
		return 0;
	}

	// initialize layout
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

		// initialize widget
		widget->visible = 1;
		widget->type = spec.type;
		widget->elem = elem;
		widget->z = spec.z;
		widget->on_click = spec.on_click;

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

	text_set_string(text_upgrade_weapon, "Buy upgrade");
	w_upgrades_weapon_btn_label.elem->width = text_upgrade_weapon->width;
	w_upgrades_weapon_btn_label.elem->height = text_upgrade_weapon->height;

	printf("UI initialized\n");

	return 1;
}

void
ui_cleanup(void)
{
	// destroy text objects
	text_destroy(text_fps);
	text_destroy(text_render_time);
	text_destroy(text_credits);
	text_destroy(text_upgrade_weapon);

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

	// update local UI state
	ui_state.show_upgrades_win = state->show_upgrades_shop;
	(
		w_upgrades_win.visible =
		w_upgrades_weapon_frame.visible =
		w_upgrades_weapon_btn.visible =
		w_upgrades_weapon_btn_label.visible =
		ui_state.show_upgrades_win
	);

	// compute the new layout
	if (!element_compute_layout(w_root.elem)) {
		return 0;
	}

	prev_state = *state;

	return 1;
}

void
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
				w->text.color
			);
			break;
		case WIDGET_IMAGE:
			render_list_add_image(
				rndr_list,
				w->image,
				w->elem->x,
				w->elem->y,
				w->z,
				w->elem->width,
				w->elem->height
			);
			break;
		}
	}
}
/*
static int
dispatch_click(struct Button *buttons, unsigned count, int x, int y)
{
	for (size_t i = 0; i < count; i++) {
		int x1 = buttons[i].x, x2 = buttons[i].x + buttons[i].w;
		int y1 = buttons[i].y, y2 = buttons[i].y + buttons[i].h;

		if (x >= x1 && x <= x2 && y >= y1 && y <= y2) {
			if (!buttons[i].on_click()) {
				return 0;
			}
		}
	}
	return 1;
}
*/

int
ui_handle_click(int x, int y)
{
	/*
	if (ui_state.show_upgrades_win) {
		return dispatch_click(
			upgrades_win_buttons,
			sizeof(upgrades_win_buttons) / sizeof(struct Button),
			x,
			y
		);
	}
	*/
	return 1;
}

static int
on_upgrade_weapon_btn_click(void)
{
	printf("hello world\n");
	return 1;
}