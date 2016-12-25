#include "list.h"
#include <assert.h>
#include <stdlib.h>

struct List*
list_new(void)
{
	struct List *list = malloc(sizeof(struct List));
	if (!list) {
		return NULL;
	}
	list->len = 0;
	list->head = NULL;
	return list;
}

void
list_destroy(struct List *list)
{
	if (list) {
		struct ListNode *node = list->head, *tmp;
		while (node) {
			tmp = node;
			node = node->next;
			free(tmp);
		}
		free(list);
	}
}

int
list_add(struct List *list, void *data)
{
	assert(list != NULL);
	struct ListNode *node = malloc(sizeof(struct ListNode));
	if (!node) {
		return 0;
	}
	node->next = list->head;
	node->data = data;
	list->head = node;
	list->len++;
	return 1;
}

int
list_remove(struct List *list, void *data, CompareFunc cmp)
{
	assert(list != NULL);
	struct ListNode *node = list->head, *prev = NULL;
	while (node) {
		if (cmp(node->data, data) == 0) {
			if (prev) {
				prev->next = node->next;
			} else {
				list->head = node->next;
			}
			free(node);
			list->len--;
			return 1;
		}
		prev = node;
		node = node->next;
	}
	return 0;
}

void
list_filter(struct List *list, FilterFunc filter, void *userdata)
{
	assert(list != NULL);
	struct ListNode *node = list->head, *prev = NULL, *tmp;
	while (node) {
		if (filter(node->data, userdata) == 0) {
			if (prev) {
				prev->next = node->next;
			} else {
				list->head = node->next;
			}
			tmp = node;
			node = node->next;
			list->len--;
			free(tmp);
		} else {
			prev = node;
			node = node->next;
		}
	}
}

void
list_foreach(struct List *list, OpFunc op, void *userdata)
{
	assert(list != NULL);
	struct ListNode *node = list->head;
	while (node) {
		if (!op(node->data, userdata)) {
			break;
		}
		node = node->next;
	}
}