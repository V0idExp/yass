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

struct Element {
	struct Anchors {
		Anchor left, right, top, bottom, hcenter, vcenter;
	} anchors;

	struct Margins {
		int left, right, top, bottom;
	} margins;

	unsigned width, height;
	int x, y;

	// read-only
	struct Element *parent;
	struct List *children;
};

struct Element*
element_new(unsigned width, unsigned height);

void
element_destroy(struct Element *elem);

int
element_compute_layout(struct Element *elem);

int
element_add_child(struct Element *elem, struct Element *child);

void
element_remove_child(struct Element *elem, struct Element *child);