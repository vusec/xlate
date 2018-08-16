#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include <sys/mman.h>

#include <xlate/macros.h>
#include <xlate/page_set.h>
#include <xlate/shm.h>
#include <xlate/tlb.h>

int build_tlb(struct page_set *tlb_set, const char *path, size_t nentries)
{
	char *area, *p;
	size_t i;
	int fd;

	if ((fd = shm_open_or_create(path, 4 * KIB)) < 0)
		return -1;

	if ((area = mmap(NULL, nentries * 4 * KIB, PROT_NONE,
		MAP_ANONYMOUS | MAP_PRIVATE, -1, 0)) == MAP_FAILED)
		goto err_close;

	page_set_init(tlb_set, nentries);

	p = area;

	for (i = 0; i < nentries; ++i) {
		if (mmap(p, 4 * KIB, PROT_READ, MAP_FIXED | MAP_SHARED, fd, 0) ==
			MAP_FAILED)
			goto err_unmap;

		page_set_push(tlb_set, p);
		p += 4 * KIB;
	}

	close(fd);

	return 0;

err_unmap:
	munmap(area, nentries * 4 * KIB);
err_close:
	close(fd);
	return -1;
}
