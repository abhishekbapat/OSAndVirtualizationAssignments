/*
 * boot.c - a kernel template (Assignment 1, ECE 6504)
 * Copyright 2021 Ruslan Nikolaev <rnikola@vt.edu>
 */

//TODO: Figure out how to correctly draw.
void kernel_start(unsigned int *framebuffer, int width, int height)
{
	/* Draw some simple figure (rectangle, square, etc)
	   to indicate kernel activity. */

	int colour = 0x2596beff;
	int centerHeight = (height/2)-1;
	int centerWidth = (width/2)-1;
	int rectHeight = 500;
	int rectWidth = 700;

	for (int i = centerWidth-(rectWidth/2); i < centerWidth+(rectWidth/2); i++)
	{
		for (int j = centerHeight-(rectHeight/2); j < centerHeight+(rectHeight/2); j++)
		{
			framebuffer[i + width * j] = colour;
		}
	}

	// for (int i = 0; i < rectWidth; i++)
	// {
	// 	for (int j = 0; j < rectHeight; j++)
	// 	{
	// 		framebuffer[i + width * j] = colour;
	// 	}
	// }

	/* Never exit! */
	while (1)
	{
	};
}
