#pragma once

#include <xlate/page_set.h>

int build_tlb(struct page_set *tlb_set, const char *path, size_t nentries);
