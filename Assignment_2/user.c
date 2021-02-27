/*
 * user.c - an application template (Assignment 2, ECE 6504)
 * Copyright 2021 Ruslan Nikolaev <rnikola@vt.edu>
 */

#include <syscall.h>

void user_start(void)
{
	// TODO: place some system call here

	// Cast all arguments, including pointers, to 'long'
	// Note that a string is const char * (a pointer)

	/* Never exit */
	while (1) {};
}
