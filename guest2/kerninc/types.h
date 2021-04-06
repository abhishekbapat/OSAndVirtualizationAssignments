#pragma once

typedef signed long long ssize_t;
typedef unsigned long long size_t;

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed long long int64_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

#define SIZE_MAX 0xFFFFFFFFFFFFFFFFULL
typedef long long intptr_t;
typedef unsigned long long uintptr_t;

#define NULL ((void *)0)

typedef struct information // This struct is used while passing information from bootloader to kernel.
{
	uintptr_t kernel_stack_buffer;
	uintptr_t user_stack_buffer;
	uintptr_t kernel_pt_base;
	uintptr_t user_pt_base;
	uintptr_t user_app_buffer;
	uintptr_t tss_stack_buffer;
	uintptr_t tss_segment_buffer;
	uintptr_t extra_page_for_exception;
	uintptr_t tls_buffer;
	uintptr_t shared_page;
	uint32_t num_user_ptes;
	uint32_t num_user_pdes;
	uint32_t num_user_pdpes;
	uint32_t num_user_pml4es;
	uint32_t num_kernel_stack_pages;
	uint32_t num_user_stack_pages;
	uint32_t num_user_binary_pages;
} information;

struct tls_block
{
	uintptr_t myself;
	char padding[4088];
};
typedef struct tls_block tls_block_t;

/* Xen/KVM per-vcpu time ABI. */
struct pvclock_vcpu_time_info {
	uint32_t version;
	uint32_t pad0;
	uint64_t tsc_timestamp;
	uint64_t system_time;
	uint32_t tsc_to_system_mul;
	int8_t tsc_shift;
	uint8_t flags;
	uint8_t pad[2];
} __attribute__((__packed__));
typedef struct pvclock_vcpu_time_info pvclock_vcpu_time_info_t; 

/* Xen/KVM wall clock ABI. */
struct pvclock_wall_clock {
	uint32_t version;
	uint32_t sec;
	uint32_t nsec;
} __attribute__((__packed__));
typedef struct pvclock_wall_clock pvclock_wall_clock_t;