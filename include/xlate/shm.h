#pragma once

#include <stdlib.h>

int shm_populate(const char *path, size_t size);
int shm_open_or_create(const char *path, size_t size);
