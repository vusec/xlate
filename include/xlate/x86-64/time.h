#pragma once

static inline uint64_t rdtsc(void)
{
	uint64_t lo, hi;

	asm volatile("rdtscp\n"
		: "=a" (lo), "=d" (hi)
		:: "%rcx");

	lo |= (hi << 32);

	return lo;
}
