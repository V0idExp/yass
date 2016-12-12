#include "error.h"
#include "memory.h"
#include "widget.h"

struct Widget*
widget_new(void)
{
	struct Widget *widget = make(struct Widget);
	if (!widget) {
		return NULL;
	}

	glGenVertexArrays(1, &widget->vao);
	if (glGetError() != GL_NO_ERROR || !widget->vao) {
		error(ERR_OPENGL);
		widget_destroy(widget);
		return NULL;
	}

	return widget;
}

void
widget_destroy(struct Widget *widget)
{
	if (widget) {
		glDeleteVertexArrays(1, &widget->vao);
		destroy(widget);
	}
}