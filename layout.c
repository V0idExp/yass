#include "error.h"
#include "layout.h"
#include "list.h"
#include "memory.h"
#include "utils.h"

Measure
measure_pc(short pc)
{
	struct Measure m = {
		.unit = MEASURE_UNIT_PC,
		.value = pc
	};
	return m;
}

Measure
measure_px(short px)
{
	struct Measure m = {
		.unit = MEASURE_UNIT_PX,
		.value = px
	};
	return m;
}

struct Element*
element_new(Measure width, Measure height)
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

static int
destroy_elem_list_op(void *data, void *userdata)
{
	element_destroy(data);
	return 1;
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
		return elem->y + elem->height.computed;
	case ANCHOR_LEFT:
		return elem->x;
	case ANCHOR_RIGHT:
		return elem->x + elem->width.computed;
	case ANCHOR_HCENTER:
		return elem->x + elem->width.computed / 2;
	case ANCHOR_VCENTER:
		return elem->y + elem->height.computed / 2;
	}
	return 0; // should be never reached
}

static int
compute_measure(struct Element *elem, int horizontal, Measure m)
{
	switch (m.unit) {
	case MEASURE_UNIT_PC:
		return m.value / 100.f * (horizontal ? elem->width.computed : elem->height.computed);
	case MEASURE_UNIT_PX:
		return m.value;
	}
	return 0;
}

static int
compute_size(struct Element *elem)
{
	// compute width
	if (elem->anchors.left && elem->anchors.right && elem->parent) {
		int left = (
			get_anchor_pos(elem->parent, elem->anchors.left) +
			compute_measure(elem->parent, 1, elem->margins.left)
		);
		int right = (
			get_anchor_pos(elem->parent, elem->anchors.right) -
			compute_measure(elem->parent, 1, elem->margins.right)
		);

		if (left > right) {
			return 0;
		}
		elem->width.computed = right - left;
	} else if (elem->width.unit == MEASURE_UNIT_PX ||
	           (elem->width.unit == MEASURE_UNIT_PC && elem->parent)) {
		elem->width.computed = compute_measure(
			elem->parent,
			1,
			elem->width
		);
	} else {
		return 0;
	}

	// compute height
	if (elem->anchors.top && elem->anchors.bottom && elem->parent) {
		int top = (
			get_anchor_pos(elem->parent, elem->anchors.top) +
			compute_measure(elem->parent, 0, elem->margins.top)
		);
		int bottom = (
			get_anchor_pos(elem->parent, elem->anchors.bottom) -
			compute_measure(elem->parent, 0, elem->margins.bottom)
		);

		if (top > bottom) {
			return 0;
		}
		elem->height.computed = (bottom - top);
	} else if (elem->height.unit == MEASURE_UNIT_PX ||
	           (elem->height.unit == MEASURE_UNIT_PC && elem->parent)) {
		elem->height.computed = compute_measure(
			elem->parent,
			0,
			elem->height
		);
	} else {
		return 0;
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
			offset = compute_measure(
				elem->parent,
				1,
				elem->margins.left
			);
		} else if (elem->anchors.right) {
			a = elem->anchors.right;
			offset = -(
				elem->width.computed +
				compute_measure(
					elem->parent,
					1,
					elem->margins.right
				)
			);
		} else if (elem->anchors.hcenter) {
			a = elem->anchors.hcenter;
			offset = -(int)elem->width.computed / 2;
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
			offset = compute_measure(
				elem->parent,
				0,
				elem->margins.top
			);
		} else if (elem->anchors.bottom) {
			a = elem->anchors.bottom;
			offset = -(
				elem->height.computed +
				compute_measure(
					elem->parent,
					0,
					elem->margins.bottom
				)
			);
		} else if (elem->anchors.vcenter) {
			a = elem->anchors.vcenter;
			offset = -(int)elem->height.computed / 2;
		}

		if (a) {
			elem->y = get_anchor_pos(elem->parent, a) + offset;
		}
	}

	return 1;
}

static int
compute_elem_layout_op(void *data, void *userdata)
{
	int *ok = userdata;
	return (*ok &= element_compute_layout(data));
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

struct TraverseContext {
	int (*func)(struct Element *e, void *userdata);
	void *userdata;
};

static int
traverse_elem_op(void *data, void *userdata)
{
	struct Element *elem = data;
	struct TraverseContext *traverse = userdata;
	return element_traverse(
		elem,
		traverse->func,
		traverse->userdata
	);
}

int
element_traverse(
	struct Element *elem,
	int (*func)(struct Element *e, void *userdata),
	void *userdata
) {
	if (!func(elem, userdata)) {
		return 0;
	}
	struct TraverseContext traverse = {
		.func = func,
		.userdata = userdata,
	};
	list_foreach(elem->children, traverse_elem_op, &traverse);
	return 1;
}