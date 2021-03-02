/*
 * This kernel initializes a 4gb page table mapping for the amd64 architecture.
 * All values are hardcoded accordingly.
 * After page table initialization, this kernel draws a rectanlge in the video frambuffer passed by the bootloader.
 */

#include <fb.h>
#include <printf.h>
#include <types.h>
#include <kernel_syscall.h>

// Declare the methods.
uintptr_t page_table_init(information);
void write_cr3(uintptr_t);

// Kernel entry point.
void kernel_start(void *kernel_stack_buffer, unsigned int *framebuffer, unsigned int width, unsigned int height, information *info)
{
	uintptr_t user_virt_addr = 0xFFFFFFFFC0001000; // Calculated manually according to the page table setup.
	user_stack = (void *)info->user_stack_buffer;
	uintptr_t topAddr = page_table_init(*info);
	write_cr3(topAddr);

	fb_init(framebuffer, width, height);

	printf("User stack pages: %d \n", info->num_user_stack_pages);
	printf("Kernel pt base pointer: %p \n", info->kernel_pt_base);
	printf("Framebuffer ptr: %p \n", framebuffer);
	printf("Num user ptes: %d \n", info->num_user_ptes);
	printf("Num user pdes: %d \n", info->num_user_pdes);
	printf("Num user pdpes: %d \n", info->num_user_pdpes);
	printf("User virtual addr: %p \n", (void *)user_virt_addr);
	user_jump((void *)user_virt_addr);

	/* Never exit! */
	while (1)
	{
	};
}

// Initialize 4-level page table to map 4gb memory.
uintptr_t page_table_init(information info)
{
	void *kernel_pt_base = (void *)info.kernel_pt_base;
	void *user_pt_base = (void *)info.user_pt_base;
	unsigned int num_k_pte = 1048576;		   //hardcode for 4gb mapping with 4kB page size.
	unsigned int num_k_pde = num_k_pte / 512;  // 2048
	unsigned int num_k_pdpe = num_k_pde / 512; // 4
	unsigned int num_pml4 = num_k_pdpe / 4;	   // 1

	uintptr_t *k_pte = (uintptr_t *)kernel_pt_base;
	for (unsigned int i = 0; i < num_k_pte; i++)
	{
		k_pte[i] = ((0x1000 * i) + 0x3);
	}

	uintptr_t *u_pte = (uintptr_t *)user_pt_base;
	for (unsigned int i = 0; i < info.num_user_stack_pages; i++)
	{
		u_pte[i] = (0x1000 * i) + (info.user_stack_buffer-4096) + 0x7;
	}
	for (unsigned int i = info.num_user_stack_pages; i < info.num_user_ptes; i++)
	{
		unsigned int temp = i - info.num_user_stack_pages;
		u_pte[i] = (0x1000 * temp) + info.user_app_buffer + 0x7;
	}
	for (unsigned int i = info.num_user_ptes; i < 512; i++) //Assuming num_user_ptes will be lesser than 512.
	{
		u_pte[i] = 0x0ULL;
	}

	uintptr_t *k_pde = (uintptr_t *)(k_pte + num_k_pte);
	uintptr_t page_addr;
	for (int j = 0; j < num_k_pde; j++)
	{
		uintptr_t *pte_start = k_pte + 512 * j;
		page_addr = (uintptr_t)pte_start;
		k_pde[j] = page_addr + 0x3;
	}

	uintptr_t *u_pde = (uintptr_t *)(u_pte + 512);
	for (int j = 0; j < info.num_user_pdes; j++)
	{
		uintptr_t *pte_start = u_pte + 512 * j;
		page_addr = (uintptr_t)pte_start;
		u_pde[j] = page_addr + 0x7;
	}
	for (int j = info.num_user_pdes; j < 512; j++)
	{
		u_pde[j] = 0x0ULL;
	}

	uintptr_t *k_pdpe = (uintptr_t *)(k_pde + num_k_pde);
	for (int k = 0; k < num_k_pdpe; k++)
	{
		uintptr_t *pde_start = k_pde + 512 * k;
		page_addr = (uintptr_t)pde_start;
		k_pdpe[k] = page_addr + 0x3;
	}
	for (int k = num_k_pdpe; k < 512; k++)
	{
		k_pdpe[k] = 0x0ULL;
	}

	uintptr_t *u_pdpe = (uintptr_t *)(u_pde + 512);
	for (int k = 0; k < 512 - info.num_user_pdpes; k++)
	{
		u_pdpe[k] = 0x0ULL;
	}
	u_pdpe[511] = (uintptr_t)(u_pde) + 0x7;

	uintptr_t *pml4e = (uintptr_t *)(k_pdpe + 512);
	for (int m = 0; m < num_pml4; m++)
	{
		uintptr_t *pdpe_start = k_pdpe + 512 * m;
		page_addr = (uintptr_t)pdpe_start;
		pml4e[m] = page_addr + 0x3;
	}
	for (int m = num_pml4; m < 511; m++)
	{
		pml4e[m] = 0x0ULL;
	}
	pml4e[511] = (uintptr_t)(u_pdpe) + 0x7;

	return ((uintptr_t)pml4e);
}

// Write value to the cr3 register.
void write_cr3(uintptr_t cr3_value)
{
	asm volatile("mov %0, %%cr3" ::"r"(cr3_value)
				 : "memory");
}