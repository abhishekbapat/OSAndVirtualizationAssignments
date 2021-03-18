/*
 * user.c - an application template (Assignment 2, ECE 6504)
 * Copyright 2021 Ruslan Nikolaev <rnikola@vt.edu>
 */

#include <syscall.h>

__thread int a[100];

void user_start(void)
{
	const long call_type_print_message = 0; // 0 = print string associated with arg passed.
	const long call_type_print_value = 1;	// 1 = print the value passed as is.
	const int temp = 100;
	const char *message1 = "Hello this is syscall1.\n";

	for (int i = 0; i < 100; i++)
		a[i] = i;

	__syscall1(call_type_print_message, (long)message1);
	__syscall1(call_type_print_value, (long)temp);

	*((char *)0xFFFFFFFFC01FF000ULL) = 0; // Inducing a page fault. Address corresponds to 511th offset of PTE which will be set to 0x0ULL.

	const char *message2 = "Page fault handled. Extra page allocated to the location.\n";
	__syscall1(call_type_print_message, (long)message2);

	/* Never exit */
	while (1)
	{
	};
}