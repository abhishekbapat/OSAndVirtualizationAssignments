/*
 * user.c - an application template (Assignment 2, ECE 6504)
 * Copyright 2021 Ruslan Nikolaev <rnikola@vt.edu>
 */

#include <syscall.h>

long make_syscall(long, long, long, long, long, long);

void user_start(void)
{
	// TODO: place some system call here

	// Cast all arguments, including pointers, to 'long'
	// Note that a string is const char * (a pointer)

	//long call_type = 0; // 0 = write.
	char *message1 = "Hello this is syscall1.\n";
	char *message2 = "Hello this is syscall2.\n";

	make_syscall(1, (long)message1, 0x0, 0x0, 0x0, 0x0);
	make_syscall(1, (long)message2, 0x0, 0x0, 0x0, 0x0);
	/* Never exit */
	while (1)
	{
	};
}

long make_syscall(long call_type, long a1, long a2, long a3, long a4, long a5)
{
	long ret;
	asm volatile("movq %0, %%rax" ::"r"(call_type));
	asm volatile("movq %0, %%rdi" ::"r"(call_type));
	asm volatile("movq %0, %%rsi" ::"r"(a1));
	asm volatile("movq %0, %%rdx" ::"r"(a2));
	asm volatile("movq %0, %%r10" ::"r"(a3));
	asm volatile("movq %0, %%r8" ::"r"(a4));
	asm volatile("movq %0, %%r9" ::"r"(a5));
	asm volatile("syscall");
	asm volatile("movq %%rax, %0" : "=r"(ret));
	return ret;
}