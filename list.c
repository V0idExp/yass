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