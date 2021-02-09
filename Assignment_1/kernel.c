/*
 * boot.c - a kernel template (Assignment 1, ECE 6504)
 * Copyright 2021 Ruslan Nikolaev <rnikola@vt.edu>
 */

//TODO: Figure out how to correctly draw.
void draw_rect(int, int, int, int, int, unsigned int *);

void kernel_start(unsigned int *framebuffer, int width, int height)
{
	/* Draw some simple figure (rectangle, square, etc)
	   to indicate kernel activity. */

	int colour = 0x2596beff;
	int rectHeight = 200;
	int rectWidth = 400;

	draw_rect(colour, rectWidth, rectHeight, width, height, framebuffer);

	/* Never exit! */
	while (1)
	{
	};
}

void draw_rect(int colour, int rectWidth, int rectHeight, int width, int height, unsigned int *framebuffer)
{
	int centerHeight = (height / 2) - 1;
	int centerWidth = (width / 2) - 1;

	for (int i = centerWidth - (rectWidth / 2); i < centerWidth + (rectWidth / 2); i++)
	{
		for (int j = centerHeight - (rectHeight / 2); j < centerHeight + (rectHeight / 2); j++)
		{
			framebuffer[i + width * j] = colour;
		}
	}
}