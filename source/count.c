#include <ctype.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sched.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <xlate/eviction.h>
#include <xlate/list.h>
#include <xlate/macros.h>
#include <xlate/page_set.h>
#include <xlate/shm.h>
#include <xlate/string.h>
#include <xlate/termio.h>
#include <xlate/tlb.h>

#include <xlate/x86-64/clflush.h>
#include <xlate/x86-64/time.h>
#include <xlate/x86-64/tsx.h>

static char *send_page = NULL;
static char *recv_page = NULL;
static struct list wsets[19];
static struct page_set xsets[19];
static struct page_set tlb_set;

int send_raw(uint32_t word)
{
	size_t i;

	for (size_t j = 0; j < 100; ++j) {
		for (i = 0; i < 19; ++i) {
			if (word & BIT(i)) {
				*(volatile char *)(send_page + i * 4 * KIB);
			}
		}
	}

	return 0;
}

uint32_t fr_recv_raw(void)
{
	size_t i;
	uint64_t t0, dt;
	uint32_t word = 0;

	for (i = 0; i < 19; ++i) {
		clflush(recv_page + i * 4 * KIB);
	}

	asm volatile("mfence\n" ::: "memory");

	for (i = 0; i < 19; ++i) {
		t0 = rdtsc();
		*(volatile char *)(recv_page + i * 4 * KIB);
		dt = rdtsc() - t0;

		if (dt < 120)
			word |= BIT(i);
	}

	return word;
}

uint32_t ff_recv_raw(void)
{
	size_t i;
	uint64_t t0, dt;
	uint32_t word = 0;
	void *line;

	for (i = 0; i < 19; ++i) {
		line = recv_page + i * 4 * KIB;

		asm volatile("cpuid\n" :: "a" (0) : "%rbx", "%rcx", "%rdx");
		asm volatile("mfence\n" ::: "memory");
		t0 = rdtsc();
		asm volatile("mfence\n" ::: "memory");
		clflush(line);
		asm volatile("mfence\n" ::: "memory");
		dt = rdtsc() - t0;
		asm volatile("mfence\n" ::: "memory");
		asm volatile("cpuid\n" :: "a" (0) : "%rbx", "%rcx", "%rdx");

		if (dt >= 172 && dt < 200)
			word |= BIT(i);
	}

	return word;
}

uint32_t pp_recv_raw(void)
{
	size_t i;
	uint64_t dt;
	uint32_t word = 0;

	for (i = 0; i < 19; ++i) {
		dt = prime(wsets + i);

		if (dt > 920)
			word |= BIT(i);
	}

	return word;
}

static int pp_build_eviction_sets(size_t npages, char *base)
{
	struct page_set lines;
	struct page_set wset;
	char *target;
	size_t i;

	if (build_page_pool(&lines, npages) < 0) {
		fprintf(stderr, "unable to build the page pool.\n");
		return -1;
	}

	page_set_init(&wset, 16);

	for (i = 0; i < 19; ++i) {
		target = base + i * 4 * KIB;
		find_wset(&lines, &wset, NULL, target, 16, prime_and_probe);
		page_set_link(&wset, wsets + i, 0);
	}

	fprintf(stderr, "found eviction sets\n");

	return 0;
}

uint32_t pa_recv_raw(void)
{
	struct list *wset, *node;
	size_t i;
	unsigned ret;
	uint32_t word = 0;

	for (i = 0; i < 19; ++i) {
		wset = wsets + i;

		if ((ret = xbegin()) == XBEGIN_INIT) {
			list_foreach(wset, node);

			for (volatile int x = 0; x < 2500; ++x);
			xend();
		} else if (ret & XABORT_CAPACITY) {
			word |= BIT(i);
		}
	}

	return word;
}

uint32_t xp_recv_raw(void)
{
	size_t i;
	uint64_t dt;
	uint32_t word = 0;

	for (i = 0; i < 19; ++i) {
		dt = xlate(xsets + i, &tlb_set);

		// 850 is good for cross-core.
		if (dt > 800)
			word |= BIT(i);
	}

	return word;
}

static int xp_build_eviction_sets(size_t npages, char *base)
{
	struct page_set lines;
	struct page_set *wset;
	char *target;
	size_t i;

	build_ptable_pool(&lines, "/foo", npages);
	build_tlb(&tlb_set, "/foo", 1600);

	for (i = 0; i < 19; ++i) {
		wset = xsets + i;
		target = base + i * 4 * KIB;

		find_wset(&lines, wset, &tlb_set, target, 16, xlate_and_probe);
	}

	fprintf(stderr, "found eviction sets\n");

	return 0;
}

uint32_t xa_recv_raw(void)
{
	struct page_set *wset;
	size_t i, k;
	unsigned ret;
	uint32_t word = 0;

	for (k = 0; k < 19; ++k) {
		wset = xsets + k;

		page_set_shuffle(wset);

		xlate(wset, &tlb_set);

		for (i = 0; i < tlb_set.len; ++i)
			*(volatile char *)tlb_set.data[i];

		if ((ret = xbegin()) == XBEGIN_INIT) {
			for (i = 0; i < wset->len; ++i)
				*(volatile char *)wset->data[i];

			for (volatile int x = 0; x < 2500; ++x);
			xend();
		} else if (ret & XABORT_CONFLICT) {
			word |= BIT(k);
		}
	}

	return word;
}

struct method {
	const char *name;
	int (* prepare)(size_t, char *);
	uint32_t (* recv_raw)(void);
};

static struct method *method;
struct method methods[] = {
	{ "flush-reload", NULL, fr_recv_raw },
	{ "flush-flush", NULL, ff_recv_raw },
	{ "prime-probe", pp_build_eviction_sets, pp_recv_raw },
	{ "prime-abort", pp_build_eviction_sets, pa_recv_raw },
	{ "xlate-probe", xp_build_eviction_sets, xp_recv_raw },
	{ "xlate-abort", xp_build_eviction_sets, xa_recv_raw },
	{},
};

struct method *lookup_method(const char *name)
{
	struct method *current;

	for (current = methods; current->name; ++current) {
		if (stricmp(current->name, name) == 0)
			return current;
	}

	return NULL;
}

int prepare(size_t npages, char *base)
{
	if (!method->prepare)
		return 0;

	return method->prepare(npages, base);
}

uint32_t recv_raw(void)
{
	return method->recv_raw();
}

static int seq_no = 0;
static int ack_no = 0;

int send_byte(uint8_t byte)
{
	uint32_t word;
	uint32_t edc;
	uint32_t ack;
	uint32_t expected;

	word = byte;
	word |= (seq_no & 0x3f) << 8;
	edc = 15 - __builtin_popcount(word);
	word |= (edc & 0xf) << 15;

	expected = byte;
	expected |= (seq_no & 0x3f) << 8;
	expected |= BIT(14);
	edc = 15 - __builtin_popcount(expected);
	expected |= (edc & 0xf) << 15;

	do {
		send_raw(word);
		ack = recv_raw();
	} while (ack != expected);

	seq_no = (seq_no + 21) & (64 - 1);

	return 0;
}

int recv_byte(uint8_t *byte)
{
	int cur_no;
	uint32_t ret;
	uint32_t data;
	uint32_t ack;
	uint32_t edc;

	while (1) {
		ret = recv_raw();

		data = ret & 0xff;
		cur_no = (ret >> 8) & 0x3f;
		edc = (ret >> 15) & 0xf;

		ack = data;
		ack |= (cur_no & 0x3f) << 8;
		ack |= BIT(14);
		ack |= (15 - __builtin_popcount(ack)) << 15;

		if (edc != (uint32_t)(15 - __builtin_popcount(ret & 0x7fff)))
			continue;

		send_raw(ack);

		if (cur_no == ack_no) {
			ack_no = (ack_no + 21) & (64 - 1);
			*byte = data;
			break;
		}
	}

	return 0;
}

void do_recv(void)
{
	struct timespec past, now;
	double diff;
	uint32_t count = 0;
	uint8_t byte = 0;
	size_t errors = 0;
	size_t hamming = 0;

	fprintf(stderr, "Press any key to continue...\n");
	await_key_press();
	clock_gettime(CLOCK_MONOTONIC, &past);

	for (count = 0; count < 16 * UINT8_MAX; ++count) {
		recv_byte(&byte);
		printf("0x%02x ", byte);
		fflush(stdout);

		if (byte != (count & 0xff))
			++errors;

		hamming += __builtin_popcount(byte ^ (count & 0xff));
	}

	clock_gettime(CLOCK_MONOTONIC, &now);

	diff = (now.tv_sec + now.tv_nsec * .000000001) - (past.tv_sec + past.tv_nsec * .000000001);

	fprintf(stderr, "%.02lf bytes/sec (raw)\n", count / diff);
	fprintf(stderr, "%.02lf bytes/sec (correct)\n", (count - errors) / diff);
	fprintf(stderr, "%.02lf byte errors/sec\n", errors / diff);
	fprintf(stderr, "%.02lf bit errors/sec\n", hamming / diff);

	printf("\n");
}

void do_send(void)
{
	uint32_t count = 0;

	for (count = 0; count < 32 * UINT8_MAX; ++count) {
		send_byte(count & 0xff);
	}
}

int main(int argc, char *argv[])
{
	char *base;
	int fd;

	if (argc < 4) {
		fprintf(stderr, "usage: %s <shm-file> <method> [send|recv]\n", argv[0]);
		return -1;
	}

	if ((fd = open(argv[1], O_RDONLY, 0777)) < 0) {
		fprintf(stderr, "error: unable to open the shared memory object.\n");
		return -1;
	}

	base = mmap(NULL, 2 * MIB, PROT_READ, MAP_SHARED | MAP_HUGETLB, fd, 0);
	close(fd);

	if (base == MAP_FAILED) {
		fprintf(stderr, "error: unable to map shared memory\n");
		return -1;
	}

	if (!(method = lookup_method(argv[2]))) {
		fprintf(stderr, "error: unknown method.\n");
		close(fd);
		return -1;
	}

	if (strcmp(argv[3], "send") == 0) {
		send_page = base;
		recv_page = base + 19 * 4 * KIB;
		prepare(4096, recv_page);
		do_send();
	} else if (strcmp(argv[3], "recv") == 0) {
		send_page = base + 19 * 4 * KIB;
		recv_page = base;
		prepare(4096, recv_page);
		do_recv();
	} else {
		fprintf(stderr, "error: unknown command.\n");
	}

	return 0;
}
