#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>

int shm_populate(const char *path, size_t size)
{
	char *page;
	int fd;

	if ((fd = shm_open(path, O_RDWR | O_CREAT | O_EXCL, 0777)) < 0)
		return 0;

	if (ftruncate(fd, size) < 0)
		goto err_unlink;

	if ((page = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
		0)) == MAP_FAILED)
		goto err_unlink;

	memset(page, 0x5a, size);
	close(fd);

	return 0;

err_unlink:
	close(fd);
	shm_unlink(path);

	return -1;
}

int shm_open_or_create(const char *path, size_t size)
{
	int fd;

	if ((fd = shm_open(path, O_RDONLY, 0777)) >= 0)
		return fd;

	if (shm_populate(path, size) < 0)
		return -1;

	return shm_open(path, O_RDONLY, 0777);
}
