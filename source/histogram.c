#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/mman.h>

#include <xlate/x86-64/clflush.h>
#include <xlate/x86-64/time.h>

int main(int argc, char *argv[])
{
	uint64_t t0, slow, fast;
	char *page;
	size_t i, count;

	if (argc < 2) {
		fprintf(stderr, "usage: %s <count>\n",
			argv[0]);
		return -1;
	}

	count = (size_t)strtoull(argv[1], NULL, 10);

	page = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
		MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	memset(page, 0x5a, 4096);

	for (i = 0; i < count; ++i) {
		clflush(page);

		asm volatile("mfence" ::: "memory");

		t0 = rdtsc();
		asm volatile("mfence" ::: "memory");
		//*(volatile char *)page;
		clflush(page);
		asm volatile("mfence" ::: "memory");
		slow = rdtsc() - t0;
		asm volatile("mfence" ::: "memory");

		*(volatile char *)page;
		asm volatile("mfence" ::: "memory");

		t0 = rdtsc();
		asm volatile("mfence" ::: "memory");
		//*(volatile char *)page;
		clflush(page);
		asm volatile("mfence" ::: "memory");
		fast = rdtsc() - t0;

		asm volatile("mfence" ::: "memory");

		printf("%" PRIu64 " %" PRIu64 "\n", slow, fast);
	}

	munmap(page, 4096);

	return 0;
}

