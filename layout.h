#pragma once

#include <stdint.h>

enum {
	ANCHOR_NONE,
	ANCHOR_TOP,
	ANCHOR_BOTTOM,
	ANCHOR_LEFT,
	ANCHOR_RIGHT,
	ANCHOR_VCENTER,
	ANCHOR_HCENTER
};

typedef int8_t Anchor;

enum {
	MEASURE_UNIT_PX,
	MEASURE_UNIT_PC
};

typedef struct Measure {
	int unit : 2;
	int value : 14;
	int computed;
} Measure;

Measure
measure_pc(short pc);

Measure
measure_px(short px);

struct Element {
	// element anchors
	struct Anchors {
		Anchor left, right, top, bottom, hcenter, vcenter;
	} anchors;

	// margins
	struct Margins {
		Measure left, right, top, bottom;
	} margins;

	// size
	Measure width, height;

	// position
	int x, y;

	// unmanaged userdata pointer
	void *userdata;

	// read-only
	struct Element *parent;
	struct List *children;
};

struct Element*
element_new(Measure width, Measure height);

void
element_destroy(struct Element *elem);

int
element_compute_layout(struct Element *elem);

int
element_traverse(
	struct Element *elem,
	int (*func)(struct Element *e, void *userdata),
	void *userdata
);

int
element_add_child(struct Element *elem, struct Element *child);

void
element_remove_child(struct Element *elem, struct Element *child);