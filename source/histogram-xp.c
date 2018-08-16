#include <ctype.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sched.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <xlate/list.h>
#include <xlate/macros.h>
#include <xlate/page_set.h>
#include <xlate/ptable_set.h>

#include <xlate/x86-64/clflush.h>
#include <xlate/x86-64/time.h>

struct page_set tlb_set;

int cmp_size(const void *lhs, const void *rhs)
{
	return memcmp(lhs, rhs, sizeof(size_t));
}

uint64_t xlate(struct page_set *working_set)
{
	uint64_t t0, dt = 0;
	size_t i;

	for (i = 0; i < tlb_set.len; ++i)
		*(volatile char *)tlb_set.data[i];

	t0 = rdtsc();

	for (i = 0; i < working_set->len; ++i)
		*(volatile char *)working_set->data[i];

	dt = rdtsc() - t0;

	for (i = tlb_set.len - 1; i < tlb_set.len; ++i)
		*(volatile char *)tlb_set.data[i];

	for (i = working_set->len - 1; i < working_set->len; ++i)
		*(volatile char *)working_set->data[i];

	for (i = 0; i < tlb_set.len; ++i)
		*(volatile char *)tlb_set.data[i];

	for (i = 0; i < working_set->len; ++i)
		*(volatile char *)working_set->data[i];

	for (i = tlb_set.len - 1; i < tlb_set.len; ++i)
		*(volatile char *)tlb_set.data[i];

	for (i = working_set->len - 1; i < working_set->len; ++i)
		*(volatile char *)working_set->data[i];

	return dt;
}

int xlate_and_probe(struct page_set *working_set, void *line)
{
	uint64_t times[16];
	uint64_t t0, dt;
	size_t i;

	for (i = 0; i < 16; ++i) {
		*(volatile char *)line;
		xlate(working_set);

		t0 = rdtsc();
		*(volatile char *)line;
		dt = rdtsc() - t0;

		times[i] = dt;
	}

	qsort(times, 16, sizeof *times, cmp_size);

	return times[16 / 2] > 100;
}

void *build_working_set(struct page_set *lines, struct page_set *working_set,
	void *target)
{
	void *line = NULL;
	size_t i;

	while (lines->len) {
		i = (size_t)(lrand48() % lines->len);

		line = page_set_remove(lines, i);

		if (xlate_and_probe(working_set, target))
			break;

		page_set_push(working_set, line);
	}

	return line;
}

void optimise_working_set(struct page_set *lines,
	struct page_set *working_set, void *target)
{
	struct cache_line *line;
	size_t i;

	for (i = working_set->len; i > 0; --i) {
		line = page_set_remove(working_set, i - 1);

		if (xlate_and_probe(working_set, target)) {
			page_set_push(lines, line);
		} else {
			page_set_push(working_set, line);
		}
	}
}

char *alloc_huge_page(void)
{
	char *page, *offset;
	size_t size;

	if ((page = mmap(NULL, 4 * MIB, PROT_NONE,
		MAP_ANONYMOUS | MAP_PRIVATE, -1, 0)) == MAP_FAILED)
		return NULL;

	offset = (char *)ALIGN_UP((uintptr_t)page, 2 * MIB);

	size = (size_t)(offset - page);
	munmap(page, size);

	size = (size_t)(4 * MIB - (uintptr_t)offset - 2 * MIB);
	//munmap(offset + 2 * MIB, size);

	return offset;
}

char *alloc_page(int fd)
{
	char *page;

	if (!(page = alloc_huge_page()))
		return NULL;

	mmap(page, 4 * KIB, PROT_READ | PROT_WRITE,
		MAP_FIXED | MAP_SHARED, fd, 0);
	munmap(page + 4 * KIB, 2 * MIB - 4 * KIB);

	return page;
}

void build_tlb(int fd)
{
	char *area, *p;
	size_t i;

	area = mmap(NULL, 32 * 2 * MIB, PROT_NONE,
		MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

	page_set_init(&tlb_set, 1600 + 32);

	p = area;

	for (i = 0; i < 1600; ++i) {
		mmap(p, 4 * KIB, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED,
			fd, 0);
		page_set_push(&tlb_set, p);
		p += 4 * KIB;
	}

	p = area + 8 * MIB;

	for (i = 4; i < 32; ++i) {
		mmap(p, 4 * KIB, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED,
			fd, 0);
		page_set_push(&tlb_set, p);
		p += 2 * MIB;
	}
}

int main(int argc, char *argv[])
{
	uint64_t fast, slow;
	struct page_set lines, wset;
	char *page;
	char *target;
	size_t count;
	size_t i;
	size_t npages = 4096;
	int fd;

	if (argc < 2) {
		fprintf(stderr, "usage: %s <count>\n",
			argv[0]);
		return -1;
	}

	count = (size_t)strtoull(argv[1], NULL, 10);

	if ((fd = shm_open("/foo", O_RDWR | O_CREAT | O_TRUNC, 0777)) < 0) {
		fprintf(stderr, "error: unable to open shared memory\n");
		return -1;
	}

	ftruncate(fd, 4 * KIB);

	page = mmap(NULL, 4 * KIB, PROT_READ | PROT_WRITE,
		MAP_SHARED, fd, 0);
	memset(page, 4 * KIB, 0x5a);
	munmap(page, 4 * KIB);

	build_tlb(fd);

	if ((target = mmap(NULL, 4 * KIB, PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED) {
		fprintf(stderr, "error: unable to map target page\n");
		return -1;
	}

	memset(target, 0x5b, 4 * KIB);

	page_set_init(&lines, npages);

	for (i = 0; i < npages; ++i) {
		page = alloc_page(fd);
		page_set_push(&lines, page);
	}

	page_set_init(&wset, 16);

	do {
		build_working_set(&lines, &wset, target);
		optimise_working_set(&lines, &wset, target);
		fprintf(stderr, "found working set of size: %zu\n", wset.len);
	} while (wset.len > 16);

	for (i = 0; i < count; ++i) {
		xlate(&wset);
		fast = xlate(&wset);
		*(volatile char *)target;
		slow = xlate(&wset);

		printf("%" PRIu64 " %" PRIu64 "\n", slow, fast);
	}

	close(fd);
	shm_unlink("/foo");

	return 0;
}
