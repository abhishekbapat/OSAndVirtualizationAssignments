#pragma once

static inline uint64_t
rdtsc(void)
{
	uint32_t eax, edx;
	__asm__ __volatile__("rdtsc"
						 : "=a"(eax), "=d"(edx));
	return ((uint64_t)edx << 32) | eax;
}

static inline uint64_t
mul64_32(uint64_t a, uint32_t b)
{
	uint64_t prod;
	__asm__(
		"mul %%rdx ; "
		"shrd $32, %%rdx, %%rax"
		: "=a"(prod)
		: "0"(a), "d"((uint64_t)b));

	return prod;
}

#define NSEC_PER_SEC 1000000000ULL