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
	uint32_t num_user_ptes;
	uint32_t num_user_pdes;
	uint32_t num_user_pdpes;
	uint32_t num_user_pml4es;
	uint32_t num_kernel_stack_pages;
	uint32_t num_user_stack_pages;
	uint32_t num_user_binary_pages;
} information;
