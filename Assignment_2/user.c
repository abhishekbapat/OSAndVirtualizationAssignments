/*
 * user.c - an application template (Assignment 2, ECE 6504)
 * Copyright 2021 Ruslan Nikolaev <rnikola@vt.edu>
 */

#include <syscall.h>

void user_start(void)
{
	const long call_type_print_message = 0; // 0 = print string associated with arg passed.
	const long call_type_print_value = 1; // 1 = print the value passed as is.
	const int temp = 100;
	const char *message1 = "Hello this is syscall1.\n";
	__syscall1(call_type_print_message, (long)message1);
	__syscall1(call_type_print_value, temp);

	/* Never exit */
	while (1)
	{
	};
}