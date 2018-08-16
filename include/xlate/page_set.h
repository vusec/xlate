#pragma once

#include <xlate/list.h>

struct page_set {
	void **data;
	size_t alloc, len;
};

struct cache_line {
	struct list set;
};

int page_set_resize(struct page_set *set, size_t alloc);
int page_set_init(struct page_set *set, size_t alloc);
void page_set_destroy(struct page_set *set);
int page_set_clear(struct page_set *set);
int page_set_push(struct page_set *set, void *line);
void *page_set_remove(struct page_set *set, size_t index);
int page_set_link(struct page_set *set, struct list *ret, size_t offset);
void page_set_remap(struct page_set *set, void *target);
int page_set_shuffle(struct page_set *set);
