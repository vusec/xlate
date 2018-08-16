#pragma once

#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(x, y) (((x) > (y)) ? (x) : (y))

#define BIT(n) (1 << (n))

#define KIB ((size_t)1024)
#define MIB (1024 * KIB)
#define GIB (1024 * MIB)

#define ALIGN_UP(ptr, align) \
	((((uintptr_t)ptr) & (align - 1)) ? \
	 ((((uintptr_t)ptr) + (align - 1)) & ~(align - 1)) : \
	 ((uintptr_t)ptr))
#define ALIGN_DOWN(ptr, align) \
	((ptr) & ~(align - 1))
