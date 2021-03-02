/*
 * user.c - an application template (Assignment 2, ECE 6504)
 * Copyright 2021 Ruslan Nikolaev <rnikola@vt.edu>
 */

#include <syscall.h>

long make_syscall(long, long, long, long, long, long);

void user_start(void)
{
	long call_type = 1; // 0 = write.
	const char *message1 = "Hello this is syscall1.\n";
	const char *message2 = "Hello this is syscall2.\n";

	__syscall1(call_type, (long)message1);
	__syscall1(call_type, (long)message2);

	/* Never exit */
	while (1)
	{
	};
}