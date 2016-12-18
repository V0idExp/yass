#include "font.h"
#include "renderer.h"
#include "state.h"
#include "text.h"
#include "texture.h"
#include "widget.h"
#include "game.h"

/*** UI STATE ***/
static struct {
	int show_upgrade_shop;
} ui_state = {
	0
};

/*** RESOURCES ***/
static struct Font *font_dbg = NULL;
static struct Font *font_hud = NULL;
static struct Text *fps_text = NULL;
static struct Text *render_time_text = NULL;
static struct Text *credits_text = NULL;
static struct Widget *hp_bar = NULL;
static struct Widget *hp_bar_bg = NULL;
static struct Widget *shop_win_bg = NULL;
static struct Texture *tex_hp_bar_green = NULL;
static struct Texture *tex_hp_bar_bg = NULL;
static struct Texture *tex_shop_win_bg = NULL;

// TEXTURES
static const struct TextureRes {
	const char *file;
	struct Texture **var;
} textures[] = {
	{ "data/art/UI/squareGreen.png", &tex_hp_bar_green },
	{ "data/art/UI/squareRed.png", &tex_hp_bar_bg },
	{ "data/art/UI/metalPanel_red.png", &tex_shop_win_bg },
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
	{ NULL }
};

static int
ui_init(void);

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
		printf("loaded texure `%s`\n", res.file);
	}

	// load fonts
	for (unsigned i = 0; fonts[i].file != NULL; i++) {
		if (!(*fonts[i].var = font_from_file(fonts[i].file, fonts[i].size))) {
			fprintf(
				stderr,
				"failed to load font `%s`\n",
				fonts[i].file
			);
			return 0;
		}
		printf("loaded font `%s`\n", fonts[i].file);
	}

	return ui_init();
}

void
ui_cleanup(void)
{
	widget_destroy(shop_win_bg);
	widget_destroy(hp_bar_bg);
	widget_destroy(hp_bar);
	text_destroy(fps_text);
	text_destroy(render_time_text);
	text_destroy(credits_text);

	// destroy fonts
	for (unsigned i = 0; fonts[i].file; i++) {
		font_destroy(*fonts[i].var);
	}

	// destroy textures
	for (unsigned i = 0; textures[i].file; i++) {
		texture_destroy(*textures[i].var);
	}
}

static int
ui_init(void)
{
	// create text renderables
	fps_text = text_new(font_dbg);
	render_time_text = text_new(font_dbg);
	credits_text = text_new(font_hud);
	if (!fps_text || !render_time_text || !credits_text) {
		return 0;
	}
	text_set_fmt(credits_text, "Credits: 0$");

	// HP bar
	hp_bar = widget_new();
	if (!hp_bar) {
		return 0;
	}
	hp_bar->texture = tex_hp_bar_green;
	hp_bar->width = 200;
	hp_bar->height = 26;
	hp_bar->border.left = 6;
	hp_bar->border.right = 6;

	hp_bar_bg = widget_new();
	if (!hp_bar_bg) {
		return 0;
	}
	hp_bar_bg->texture = tex_hp_bar_bg;
	hp_bar_bg->width = 200;
	hp_bar_bg->height = 26;
	hp_bar_bg->border.left = 6;
	hp_bar_bg->border.right = 6;

	// upgrade shop window
	shop_win_bg = widget_new();
	if (!shop_win_bg) {
		return 0;
	}
	shop_win_bg->texture = tex_shop_win_bg;
	shop_win_bg->width = 400;
	shop_win_bg->height = 400;
	shop_win_bg->border.left = 11;
	shop_win_bg->border.right = 11;
	shop_win_bg->border.top = 32;
	shop_win_bg->border.bottom = 13;

	return 1;
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

	// update performance widgets once per second
	static float time_acc = 0;
	time_acc += dt;
	if (time_acc >= 1.0) {
		time_acc -= 1.0;
		text_set_fmt(fps_text, "FPS: %d", state->fps);
		text_set_fmt(
			render_time_text,
			"Render time: %dms",
			state->render_time
		);
	}

	// update credits text
	if (prev_state.credits != state->credits) {
		text_set_fmt(credits_text, "Credits: %d$", state->credits);
	}

	// update hitpoints widget
	if (prev_state.hitpoints != state->hitpoints) {
		hp_bar->width = (
			200.0 *
			state->hitpoints /
			PLAYER_INITIAL_HITPOINTS
		);
	}

	// update local UI state
	ui_state.show_upgrade_shop = state->show_upgrade_shop;

	prev_state = *state;

	return 1;
}

void
ui_render(struct RenderList *rndr_list)
{
	// render FPS indicator
	render_list_add_text(
		rndr_list,
		fps_text,
		-SCREEN_WIDTH / 2,
		-SCREEN_HEIGHT / 2 + 60
	);

	// render render time indicator
	render_list_add_text(
		rndr_list,
		render_time_text,
		-SCREEN_WIDTH / 2,
		-SCREEN_HEIGHT / 2 + 80
	);

	// render credits counter
	render_list_add_text(
		rndr_list,
		credits_text,
		SCREEN_WIDTH / 2 - 150,
		-SCREEN_HEIGHT / 2 + 20
	);

	// render hitpoints widget
	render_list_add_widget(
		rndr_list,
		hp_bar_bg,
		20,
		25 - hp_bar_bg->height / 2
	);
	render_list_add_widget(
		rndr_list,
		hp_bar,
		20,
		25 - hp_bar->height / 2
	);

	// render upgrades shop window, if needed
	if (ui_state.show_upgrade_shop) {
		render_list_add_widget(
			rndr_list,
			shop_win_bg,
			SCREEN_WIDTH / 2 - shop_win_bg->width / 2,
			SCREEN_HEIGHT / 2 - shop_win_bg->height / 2
		);
	}
}
