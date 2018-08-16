#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include <sys/mman.h>

#include <xlate/eviction.h>
#include <xlate/macros.h>
#include <xlate/shm.h>
#include <xlate/sort.h>

#include <xlate/x86-64/time.h>

uint64_t prime(struct list *working_set)
{
	struct list *node;
	uint64_t t0, dt;

	t0 = rdtsc();
	list_foreach(working_set, node);
	dt = rdtsc() - t0;

	list_foreach_rev(working_set, node);
	list_foreach(working_set, node);
	list_foreach_rev(working_set, node);

	return dt;
}

int prime_and_probe_fast(struct list *working_set, void *line)
{
	uint64_t times[16];
	uint64_t t0, dt;
	size_t i;

	for (i = 0; i < 16; ++i) {
		*(volatile char *)line;
		prime(working_set);

		t0 = rdtsc();
		*(volatile char *)line;
		dt = rdtsc() - t0;

		times[i] = dt;
	}

	qsort_u64(times, 16);

	return times[16 / 2] > 200;
}

int prime_and_probe(struct page_set *working_set, struct page_set *tlb_set,
	void *line)
{
	struct list set;

	(void)tlb_set;

	page_set_link(working_set, &set, (size_t)line & (4 * KIB - 1));

	return prime_and_probe_fast(&set, line);
}

uint64_t xlate(struct page_set *working_set, struct page_set *tlb_set)
{
	uint64_t t0, dt = 0;
	size_t i;

	for (i = 0; i < tlb_set->len; ++i)
		*(volatile char *)tlb_set->data[i];

	t0 = rdtsc();

	for (i = 0; i < working_set->len; ++i)
		*(volatile char *)working_set->data[i];

	dt = rdtsc() - t0;

	for (i = working_set->len - 1; i < working_set->len; --i)
		*(volatile char *)working_set->data[i];

	for (i = 0; i < working_set->len; ++i)
		*(volatile char *)working_set->data[i];

	for (i = working_set->len - 1; i < working_set->len; --i)
		*(volatile char *)working_set->data[i];

	return dt;
}

int xlate_and_probe(struct page_set *working_set, struct page_set *tlb_set,
	void *line)
{
	uint64_t times[16];
	uint64_t t0, dt;
	size_t i;

	for (i = 0; i < 16; ++i) {
		*(volatile char *)line;
		xlate(working_set, tlb_set);

		t0 = rdtsc();
		*(volatile char *)line;
		dt = rdtsc() - t0;

		times[i] = dt;
	}

	qsort_u64(times, 16);

	return times[16 / 2] > 150;
}

void *build_wset(struct page_set *pool, struct page_set *wset,
	struct page_set *tlb_set, void *target,
	int (* evicts)(struct page_set *, struct page_set *, void *))
{
	void *line = NULL;
	size_t i;

	if (!target) {
		i = (size_t)(lrand48() % pool->len);
		target = page_set_remove(pool, i);
	}

	while (pool->len) {
		i = (size_t)(lrand48() % pool->len);

		line = page_set_remove(pool, i);
		page_set_push(wset, line);
		page_set_shuffle(wset);

		if (evicts(wset, tlb_set, target))
			break;
	}

	return target;
}

void optimise_wset(struct page_set *pool, struct page_set *wset,
	struct page_set *tlb_set, void *target,
	int (*evicts)(struct page_set *, struct page_set *, void *))
{
	struct cache_line *line;
	size_t i;

	for (i = wset->len - 1; i < wset->len; --i) {
		line = page_set_remove(wset, i);

		if (evicts(wset, tlb_set, target)) {
			page_set_push(pool, line);
		} else {
			page_set_push(wset, line);
		}
	}
}

int validate_wset(struct page_set *wset, struct page_set *tlb_set,
	void *target,
	int (* evicts)(struct page_set *, struct page_set *, void *))
{
	size_t i;
	size_t score = 0;

	for (i = 0; i < 64; ++i) {
		if (evicts(wset, tlb_set, target))
			++score;
	}

	return score > 48;
}

void *find_wset(struct page_set *pool, struct page_set *wset,
	struct page_set *tlb_set, void *target, size_t nways,
	int (* evicts)(struct page_set *, struct page_set *, void *))
{
	if (!pool || !wset)
		return NULL;

	page_set_clear(wset);

	do {
		page_set_shuffle(pool);
		target = build_wset(pool, wset, tlb_set, target, evicts);

		if (wset->len < nways)
			continue;

		optimise_wset(pool, wset, tlb_set, target, evicts);
		fprintf(stderr, "built eviction set of size %zu\n", wset->len);
	} while (wset->len != nways || !validate_wset(wset, tlb_set, target, evicts));

	return target;
}

int limit_wset(struct page_set *lines, struct page_set *wset, size_t nways)
{
	void *line;

	while (wset->len > nways) {
		line = page_set_remove(wset, wset->len - 1);
		page_set_push(lines, line);
	}

	return 0;
}

int build_page_pool(struct page_set *pool, size_t npages)
{
	char *area, *p;
	size_t i;

	if ((area = mmap(NULL, npages * 4 * KIB, PROT_READ | PROT_WRITE,
		MAP_ANONYMOUS | MAP_PRIVATE, -1, 0)) == MAP_FAILED)
		return -1;

	memset(area, 0x5a, npages * 4 * KIB);

	page_set_init(pool, npages);

	p = area;

	for (i = 0; i < npages; ++i) {
		page_set_push(pool, p);
		p += 4 * KIB;
	}

	return 0;
}

int build_ptable_pool(struct page_set *pool, const char *path, size_t npages)
{
	char *area, *p;
	size_t i, size;
	int fd;

	if ((fd = shm_open_or_create(path, 4 * KIB)) < 0)
		return -1;

	if ((area = mmap(NULL, (npages + 1) * 2 * MIB, PROT_NONE,
		MAP_ANONYMOUS | MAP_PRIVATE | MAP_NORESERVE, -1, 0)) == MAP_FAILED)
		goto err_close;

	page_set_init(pool, npages);

	p = (void *)ALIGN_UP((uintptr_t)area, 2 * MIB);
	size = p - area;

	if (size)
		munmap(area, size);

	area = p;
	p = area + npages * 2 * MIB;
	size = 2 * MIB - size;

	if (size)
		munmap(p, size);

	p = area;

	for (i = 0; i < npages; ++i) {
		if (mmap(p, 4 * KIB, PROT_READ, MAP_FIXED | MAP_SHARED, fd, 0) ==
			MAP_FAILED)
			goto err_unmap;

		munmap(p + 4 * KIB, 2 * MIB - 4 * KIB);

		page_set_push(pool, p);
		p += 2 * MIB;
	}

	close(fd);

	return 0;

err_unmap:
	munmap(area, npages * 2 * MIB);
err_close:
	close(fd);
	return -1;
}
