#include <stdlib.h>
#include <string.h>

#include <xlate/sort.h>

static int cmp_sz(const void *lhs, const void *rhs)
{
	return memcmp(lhs, rhs, sizeof(size_t));
}

static int cmp_u64(const void *lhs, const void *rhs)
{
	return memcmp(lhs, rhs, sizeof(uint64_t));
}

void qsort_sz(size_t *base, size_t len)
{
	qsort(base, len, sizeof *base, cmp_sz);
}

void qsort_u64(uint64_t *base, size_t len)
{
	qsort(base, len, sizeof *base, cmp_u64);
}
