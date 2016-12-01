#pragma once

typedef int (*CompareFunc)(const void *a, const void *b);

struct List {
	unsigned long len;
	struct ListNode {
		struct ListNode *next;
		void *data;
	} *head;
};

struct List*
list_new(void);

void
list_destroy(struct List *list);

int
list_add(struct List *list, void *data);

int
list_remove(struct List *list, void *data, CompareFunc cmp);