#pragma once

struct list {
	struct list *next;
	struct list *prev;
};

#define list_foreach(list, node) \
	for (node = (list)->next; node != list; node = node->next)
#define list_foreach_rev(list, node) \
	for (node = (list)->prev; node != list; node = node->prev)
#define list_foreach_safe(list, node, next) \
	for (node = (list)->next, next = node->next; node != list; node = next, \
		next = node->next)

void list_init(struct list *head);
void list_insert_before(struct list *before, struct list *node);
void list_insert_after(struct list *after, struct list *node);
void list_remove(struct list *node);
void list_push(struct list *head, struct list *node);
void list_push_left(struct list *head, struct list *node);
struct list *list_pop(struct list *head);
struct list *list_pop_left(struct list *head);
