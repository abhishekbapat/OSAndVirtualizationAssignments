/*
 * boot.c - a kernel template (Assignment 1, ECE 6504)
 * Copyright 2021 Ruslan Nikolaev <rnikola@vt.edu>
 */

void _start(unsigned int *framebuffer, int width, int height)
{
	/* TODO: Draw some simple figure (rectangle, square, etc)
	   to indicate kernel activity. */

	/* Never exit! */

	int colour = 0x2596beff;

	for (int i = 0; i < width; i++)
	{
		for (int j = 0; j < height; j++)
		{
			framebuffer[i + width * j] = colour;
		}
	}

	while (1)
	{
	};
}
