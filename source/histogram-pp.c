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

#include <xlate/eviction.h>
#include <xlate/list.h>
#include <xlate/macros.h>
#include <xlate/page_set.h>

#include <xlate/x86-64/clflush.h>
#include <xlate/x86-64/time.h>

void *build_working_set(struct page_set *lines, struct page_set *working_set,
	void *target)
{
	struct list set;
	void *line = NULL;
	size_t i;

	while (lines->len) {
		i = (size_t)(lrand48() % lines->len);

		line = page_set_remove(lines, i);
		page_set_link(working_set, &set, (size_t)target & (4 * KIB - 1));

		if (prime_and_probe(&set, target))
			break;

		page_set_push(working_set, line);
	}

	return line;
}

void optimise_working_set(struct page_set *lines,
	struct page_set *working_set, void *target)
{
	struct list set;
	struct cache_line *line;
	size_t i;

	for (i = working_set->len; i > 0; --i) {
		line = page_set_remove(working_set, i - 1);
		page_set_link(working_set, &set, (size_t)target & (4 * KIB - 1));

		if (prime_and_probe(&set, target)) {
			page_set_push(lines, line);
		} else {
			page_set_push(working_set, line);
		}
	}
}

int main(int argc, char *argv[])
{
	struct list set;
	uint64_t fast, slow;
	struct page_set lines, wset;
	char *page;
	char *target;
	size_t count;
	size_t i;
	size_t npages = 4096;

	if (argc < 2) {
		fprintf(stderr, "usage: %s <count>\n",
			argv[0]);
		return -1;
	}

	count = (size_t)strtoull(argv[1], NULL, 10);

	if ((target = mmap(NULL, 4 * KIB, PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED) {
		fprintf(stderr, "error: unable to map target page\n");
		return -1;
	}

	memset(target, 0x5b, 4 * KIB);

	if ((page = mmap(NULL, npages * 4 * KIB, PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED) {
		fprintf(stderr, "error: unable to map pages for PRIME + PROBE\n");
		return -1;
	}

	memset(page, 0x5a, npages * 4 * KIB);
	page_set_init(&lines, npages);

	for (i = 0; i < npages; ++i) {
		page_set_push(&lines, page + i * 4 * KIB);
	}

	page_set_init(&wset, 16);

	do {
		build_working_set(&lines, &wset, target);
		optimise_working_set(&lines, &wset, target);
		printf("found working set of size: %zu\n", wset.len);
	} while (wset.len > 16);

	page_set_link(&wset, &set, (size_t)target & (4 * KIB - 1));

	for (i = 0; i < count; ++i) {
		prime(&set);
		fast = prime(&set);
		*(volatile char *)target;
		slow = prime(&set);

		printf("%" PRIu64 " %" PRIu64 "\n", slow, fast);
	}

	return 0;
}
