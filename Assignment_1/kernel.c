/*
 * This kernel initializes a 4gb page table mapping for the amd64 architecture.
 * All values are hardcoded accordingly.
 * After page table initialization, this kernel draws a rectanlge in the video frambuffer passed by the bootloader.
 */

#include "pagetable.h"

//Declare the methods.
void draw_rect(int, int, int, int, int, unsigned int *);
void page_table_init(unsigned int *);
struct page_table_entry page_table_entry_init_from_u64(u64);
struct page_directory_entry page_directory_entry_init_from_u64(u64);
struct page_directory_pointer_entry page_directory_pointer_entry_init_from_u64(u64);
struct page_map_level4_entry page_map_level4_entry_init_from_u64(u64);

void kernel_start(unsigned int *framebuffer, int width, int height, unsigned int *pt_base)
{
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

void page_table_init(unsigned int *base)
{
	unsigned int pages = 1048576; //hardcode for 4gb mapping.
	unsigned int num_pte = pages / 512;
	unsigned int num_pde = num_pte / 512;
	unsigned int num_pdpe = num_pde / 4;

	struct page_table_entry *pte = (struct page_table_entry *)base;
	pte[0] = page_table_entry_init_from_u64(0x0);
	for (int i = 1; i < pages; i++)
	{
		pte[i] = page_table_entry_init_from_u64((0x1000 * i) + 0x3);
	}

	struct page_directory_entry *pde = (struct page_directory_entry *)(pte + pages);
	for (int j = 0; j < num_pte; j++)
	{
		struct page_table_entry *pte_start = pte + (512 * j);
		
	}
}

void write_cr3(unsigned long long cr3_value)
{
	asm volatile("mov %0, %%cr3" ::"r"(cr3_value)
				 : "memory");
}

struct page_table_entry page_table_entry_init_from_u64(u64 value)
{
	struct page_table_entry pte;
	pte.present = value & 0x0000000000000001;
	pte.read_write = value & 0x0000000000000002;
	pte.user_supervisor = value & 0x0000000000000004;
	pte.page_level_writethough = value & 0x0000000000000008;
	pte.page_level_cache_disable = value & 0x0000000000000010;
	pte.access = value & 0x0000000000000020;
	pte.dirty = value & 0x0000000000000040;
	pte.pat = value & 0x0000000000000080;
	pte.global_page = value & 0x0000000000000100;
	pte.avl = value & 0x0000000000000E00;
	pte.page_address = value & 0x000FFFFFFFFFF000;
	pte.avail = value & 0x07F0000000000000;
	pte.pke = value & 0x7800000000000000;
	pte.nonexecute = value & 0x8000000000000000;
	return pte;
}

struct page_directory_entry page_directory_entry_init_from_u64(u64 value)
{
	struct page_directory_entry pde;
	pde.present = value & 0x0000000000000001;
	pde.read_write = value & 0x0000000000000002;
	pde.user_supervisor = value & 0x0000000000000004;
	pde.page_level_writethough = value & 0x0000000000000008;
	pde.page_level_cache_disable = value & 0x0000000000000010;
	pde.access = value & 0x0000000000000020;
	pde.ign = value & 0x00000000000001C0;
	pde.avl = value & 0x0000000000000E00;
	pde.page_address = value & 0x000FFFFFFFFFF000;
	pde.avail = value & 0x7FF0000000000000;
	pde.nonexecute = value & 0x8000000000000000;
	return pde;
}

struct page_directory_pointer_entry page_directory_pointer_entry_init_from_u64(u64 value)
{
	struct page_directory_pointer_entry pdpe;
	pdpe.present = value & 0x0000000000000001;
	pdpe.read_write = value & 0x0000000000000002;
	pdpe.user_supervisor = value & 0x0000000000000004;
	pdpe.page_level_writethough = value & 0x0000000000000008;
	pdpe.page_level_cache_disable = value & 0x0000000000000010;
	pdpe.access = value & 0x0000000000000020;
	pdpe.ign = value & 0x00000000000001C0;
	pdpe.avl = value & 0x0000000000000E00;
	pdpe.page_address = value & 0x000FFFFFFFFFF000;
	pdpe.avail = value & 0x7FF0000000000000;
	pdpe.nonexecute = value & 0x8000000000000000;
	return pdpe;
}

struct page_map_level4_entry page_map_level4_entry_init_from_u64(u64 value)
{
	struct page_map_level4_entry pml4;
	pml4.present = value & 0x0000000000000001;
	pml4.read_write = value & 0x0000000000000002;
	pml4.user_supervisor = value & 0x0000000000000004;
	pml4.page_level_writethough = value & 0x0000000000000008;
	pml4.page_level_cache_disable = value & 0x0000000000000010;
	pml4.access = value & 0x0000000000000020;
	pml4.ign = value & 0x0000000000000040;
	pml4.mbz = value & 0x0000000000000180;
	pml4.avl = value & 0x0000000000000E00;
	pml4.page_address = value & 0x000FFFFFFFFFF000;
	pml4.avail = value & 0x7FF0000000000000;
	pml4.nonexecute = value & 0x8000000000000000;
	return pml4;
}