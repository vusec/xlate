#pragma once

#include <stdint.h>
#include <stdlib.h>

#include <xlate/list.h>
#include <xlate/page_set.h>

uint64_t prime(struct list *working_set);
int prime_and_probe(struct page_set *working_set, struct page_set *tlb_set,
	void *line);
uint64_t xlate(struct page_set *working_set, struct page_set *tlb_set);
int xlate_and_probe(struct page_set *working_set, struct page_set *tlb_set,
	void *line);

void *build_wset(struct page_set *pool, struct page_set *wset,
	struct page_set *tlb_set, void *target,
	int (* evicts)(struct page_set *, struct page_set *, void *));
void optimise_wset(struct page_set *pool, struct page_set *wset,
	struct page_set *tlb_set, void *target,
	int (*evicts)(struct page_set *, struct page_set *, void *));
int validate_wset(struct page_set *wset, struct page_set *tlb_set,
	void *target,
	int (* evicts)(struct page_set *, struct page_set *, void *));
void *find_wset(struct page_set *pool, struct page_set *wset,
	struct page_set *tlb_set, void *target, size_t nways,
	int (* evicts)(struct page_set *, struct page_set *, void *));
int limit_wset(struct page_set *lines, struct page_set *wset, size_t nways);

int build_page_pool(struct page_set *pool, size_t npages);
int build_ptable_pool(struct page_set *pool, const char *path, size_t npages);
