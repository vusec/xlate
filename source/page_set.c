#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <sys/mman.h>

#include <xlate/macros.h>
#include <xlate/page_set.h>

int page_set_resize(struct page_set *set, size_t alloc)
{
	void *data;

	if (!set)
		return -1;

	if (!(data = realloc(set->data, alloc * sizeof *set->data)))
		return -1;

	set->data = data;
	set->alloc = alloc;

	return 0;
}

int page_set_init(struct page_set *set, size_t alloc)
{
	if (!set)
		return -1;

	set->data = NULL;
	set->len = 0;
	set->alloc = alloc;

	if (!alloc)
		return 0;

	return page_set_resize(set, alloc);
}

void page_set_destroy(struct page_set *set)
{
	if (!set)
		return;

	if (set->data) {
		free(set->data);
		set->data = NULL;
	}

	set->len = 0;
	set->alloc = 0;
}

int page_set_clear(struct page_set *set)
{
	if (!set)
		return -1;

	set->len = 0;

	return 0;
}

int page_set_push(struct page_set *set, void *line)
{
	if (!set)
		return -1;

	if (set->len >= set->alloc &&
		page_set_resize(set, set->alloc ? 2 * set->alloc: 64) < 0)
		return -1;

	set->data[set->len++] = line;

	return 0;
}

void *page_set_remove(struct page_set *set, size_t index)
{
	void *line;

	if (!set || index > set->len)
		return NULL;

	line = set->data[index];

	if (index + 1 < set->len) {
		memmove(set->data + index, set->data + index + 1,
			(set->len - index - 1) * sizeof *set->data);
	}

	--set->len;

	return line;
}

int page_set_link(struct page_set *set, struct list *ret, size_t offset)
{
	struct cache_line *line;
	size_t i;

	list_init(ret);

	for (i = 0; i < set->len; ++i) {
		line = (void *)((uintptr_t)set->data[i] + offset);
		list_init(&line->set);
		list_push(ret, &line->set);
	}

	return 0;
}

void page_set_remap(struct page_set *set, void *target)
{
	uintptr_t base;
	uintptr_t offset;
	size_t i;

	offset = ((uintptr_t)target & (4 * KIB - 1)) >> 6;

	for (i = 0; i < set->len; ++i) {
		base = ALIGN_DOWN((uintptr_t)set->data[i], 2 * MIB);
		base = base + (offset << 15);

		if (base == (uintptr_t)set->data[i])
			continue;

		set->data[i] = mremap(set->data[i], 4 * KIB, 4 * KIB, MREMAP_MAYMOVE | MREMAP_FIXED, (void *)base);
	}
}

int page_set_shuffle(struct page_set *set)
{
	size_t i, j;
	void *line;

	if (!set)
		return -1;

	if (set->len < 1)
		return 0;

	for (i = set->len - 1; i < set->len; --i) {
		j = (size_t)(lrand48() % (i + 1));

		line = set->data[i];
		set->data[i] = set->data[j];
		set->data[j] = line;
	}

	return 0;
}
