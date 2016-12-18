#include "error.h"
#include "layout.h"
#include "list.h"
#include "memory.h"
#include "utils.h"

struct Element*
element_new(unsigned width, unsigned height)
{
	struct Element *elem = make(struct Element);
	if (!elem || !(elem->children = list_new())) {
		element_destroy(elem);
		return NULL;
	}
	elem->width = width;
	elem->height = height;
	return elem;
}

static void
destroy_elem_list_op(void *data, void *userdata)
{
	element_destroy(data);
}

void
element_destroy(struct Element *elem)
{
	if (elem) {
		list_foreach(elem->children, destroy_elem_list_op, NULL);
		list_destroy(elem->children);
		destroy(elem);
	}
}

static int
validate_anchors(struct Element *elem)
{
	Anchor *horizontal[3] = {
		&elem->anchors.left,
		&elem->anchors.right,
		&elem->anchors.hcenter
	};
	Anchor *vertical[3] = {
		&elem->anchors.top,
		&elem->anchors.bottom,
		&elem->anchors.vcenter
	};

	int valid = 1;
	for (short i = 0; i < 3; i++) {
		Anchor h = *horizontal[i];
		if (h) {
			valid &= (
				h != ANCHOR_TOP &&
				h != ANCHOR_BOTTOM &&
				h != ANCHOR_VCENTER
			);
		}

		Anchor v = *vertical[i];
		if (v) {
			valid &= (
				v != ANCHOR_LEFT &&
				v != ANCHOR_RIGHT &&
				v != ANCHOR_HCENTER
			);
		}
	}

	if (elem->anchors.left || elem->anchors.right) {
		valid &= !elem->anchors.hcenter;
	}
	if (elem->anchors.top || elem->anchors.bottom) {
		valid &= !elem->anchors.vcenter;
	}

	return valid;
}

static int
get_anchor_pos(struct Element *elem, Anchor a)
{
	switch (a) {
	case ANCHOR_TOP:
		return elem->y;
	case ANCHOR_BOTTOM:
		return elem->y + elem->height;
	case ANCHOR_LEFT:
		return elem->x;
	case ANCHOR_RIGHT:
		return elem->x + elem->width;
	case ANCHOR_HCENTER:
		return elem->x + elem->width / 2;
	case ANCHOR_VCENTER:
		return elem->y + elem->height / 2;
	}
	return 0; // should be never reached
}

static int
compute_size(struct Element *elem)
{
	// compute width
	if (elem->anchors.left && elem->anchors.right && elem->parent) {
		int left = (
			get_anchor_pos(elem->parent, elem->anchors.left) +
			elem->margins.left
		);
		int right = (
			get_anchor_pos(elem->parent, elem->anchors.right) -
			elem->margins.right
		);

		if (left > right) {
			return 0;
		}
		elem->width = right - left;
	}

	// compute height
	if (elem->anchors.top && elem->anchors.bottom && elem->parent) {
		int top = (
			get_anchor_pos(elem->parent, elem->anchors.top) +
			elem->margins.top
		);
		int bottom = (
			get_anchor_pos(elem->parent, elem->anchors.bottom) -
			elem->margins.bottom
		);

		if (top > bottom) {
			return 0;
		}
		elem->height = bottom - top;
	}

	return 1;
}

static int
compute_position(struct Element *elem)
{
	// compute x coordinate
	if (elem->parent) {
		Anchor a = 0;
		int offset = 0;
		if (elem->anchors.left) {
			a = elem->anchors.left;
			offset = elem->margins.left;
		} else if (elem->anchors.right) {
			a = elem->anchors.right;
			offset = -(elem->width + elem->margins.right);
		} else if (elem->anchors.hcenter) {
			a = elem->anchors.hcenter;
			offset = -(int)elem->width / 2;
		}

		if (a) {
			elem->x = get_anchor_pos(elem->parent, a) + offset;
		}
	}

	// compute y coordinate
	if (elem->parent) {
		Anchor a = 0;
		int offset = 0;
		if (elem->anchors.top) {
			a = elem->anchors.top;
			offset = elem->margins.top;
		} else if (elem->anchors.bottom) {
			a = elem->anchors.bottom;
			offset = -(elem->height + elem->margins.bottom);
		} else if (elem->anchors.vcenter) {
			a = elem->anchors.vcenter;
			offset = -(int)elem->height / 2;
		}

		if (a) {
			elem->y = get_anchor_pos(elem->parent, a) + offset;
		}
	}

	return 1;
}

static void
compute_elem_layout_op(void *data, void *userdata)
{
	int *ok = userdata;
	*ok &= element_compute_layout(data);
}

int
element_compute_layout(struct Element *elem)
{
	if (!validate_anchors(elem) ||
	    !compute_size(elem) ||
	    !compute_position(elem)) {
		error(ERR_INVALID_ANCHORS);
		return 0;
	}

	int ok = 1;
	list_foreach(elem->children, compute_elem_layout_op, &ok);
	return ok;
}

int
element_add_child(struct Element *elem, struct Element *child)
{
	child->parent = elem;
	return list_add(elem->children, child);
}

void
element_remove_child(struct Element *elem, struct Element *child)
{
	list_remove(elem->children, child, ptr_cmp);
}