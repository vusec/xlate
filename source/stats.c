#include <stdlib.h>
#include <string.h>

#include <xlate/sort.h>
#include <xlate/stats.h>

int mode_u64(uint64_t *value, size_t *count, uint64_t *data, size_t len)
{
	size_t current;
	size_t i, j;

	qsort_u64(data, len);

	for (j = 0; j < len; j = i) {
		for (i = j + 1; i < len; ++i) {
			if (data[j] != data[i])
				break;
		}

		current = i - j;

		if (*count < current) {
			*count = current;
			*value = data[j];
		}
	}

	return 0;
}


