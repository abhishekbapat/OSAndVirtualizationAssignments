/*
 * This kernel initializes a 4gb page table mapping for the amd64 architecture.
 * All values are hardcoded accordingly.
 * After page table initialization, this kernel draws a rectanlge in the video frambuffer passed by the bootloader.
 */

#include "pagetable.h"

// Declare the methods.
void draw_rect(int, int, int, int, int, unsigned int *);

// Kernel entry point.
void kernel_start(unsigned int *framebuffer, int width, int height, void *pt_base)
{
	int colour = 0x2596beff;
	int rectHeight = 200;
	int rectWidth = 400;

	u64 topAddr = page_table_init(pt_base);
	write_cr3(topAddr);

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

// Initialize 4-level page table to map 4gb memory.
u64 page_table_init(void *base)
{
	unsigned int num_pte = 1048576;		   //hardcode for 4gb mapping with 4kB page size.
	unsigned int num_pde = num_pte / 512;  // 2048
	unsigned int num_pdpe = num_pde / 512; // 4
	unsigned int num_pml4 = num_pdpe / 4;  // 1

	u64 *pte = (u64 *)base;
	for (unsigned int i = 0; i < num_pte; i++)
	{
		pte[i] = ((0x1000 * i) + 0x3);
	}

	u64 *pde = (u64 *)(pte + num_pte);
	u64 page_addr;

	for (int j = 0; j < num_pde; j++)
	{
		u64 *pte_start = pte + 512 * j;
		page_addr = (u64)pte_start;
		pde[j] = page_addr + 0x3;
	}

	u64 *pdpe = (u64 *)(pde + num_pde);
	for (int k = 0; k < num_pdpe; k++)
	{
		u64 *pde_start = pde + 512 * k;
		page_addr = (u64)pde_start;
		pdpe[k] = page_addr + 0x3;
	}
	for (int k = num_pdpe; k < 512; k++)
	{
		pdpe[k] = 0x0ULL;
	}

	u64 *pml4e = (u64 *)(pdpe + 512);
	for (int m = 0; m < num_pml4; m++)
	{
		u64 *pdpe_start = pdpe + 512 * m;
		page_addr = (u64)pdpe_start;
		pml4e[m] = page_addr + 0x3;
	}
	for (int m = num_pml4; m < 512; m++)
	{
		pml4e[m] = 0x0ULL;
	}

	return ((u64)pml4e);
}

// Write value to the cr3 register.
void write_cr3(u64 cr3_value)
{
	asm volatile("mov %0, %%cr3" ::"r"(cr3_value)
				 : "memory");
}