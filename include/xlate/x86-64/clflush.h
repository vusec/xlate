#pragma once

static inline void clflush(volatile void *p)
{
	asm volatile("clflush (%0)\n"
		:: "r" (p));
}
