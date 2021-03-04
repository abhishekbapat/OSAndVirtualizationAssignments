/*
 * This kernel initializes a 4gb page table 1:1 mapping for the kernel space.
 * Then it maps the last 1gb for user space.
 */

#include <fb.h>
#include <printf.h>
#include <types.h>
#include <kernel_syscall.h>

// Declare the methods.
uintptr_t page_table_init_kernel(information);
uintptr_t page_table_init_user(information, uintptr_t);
void write_cr3(uintptr_t);

// Kernel entry point.
void kernel_start(void *kernel_stack_buffer, unsigned int *framebuffer, unsigned int width, unsigned int height, information *info)
{
	fb_init(framebuffer, width, height);

	uintptr_t user_app_virt_addr = 0xFFFFFFFFC0001000; // Calculated manually according to the page table setup.
	uintptr_t user_stack_virt_addr = 0xFFFFFFFFC0001000; // Same as above because stack is mapped before user_app in virtual mem. As stack moves downwards we shift by 4096.
	user_stack = (void *)user_stack_virt_addr; // Initialize user stack.

	printf("Initializing page tables for kernel and user space!\n");
	uintptr_t k_pml4e_base = page_table_init_kernel(*info); // Initialize kernel page tables.
	uintptr_t u_pml4e_base = page_table_init_user(*info, k_pml4e_base); // Initialize user page tables.
	write_cr3(u_pml4e_base); // Pass the base pml4e to cr3.

	printf("Initializing system calls!\n");
	syscall_init(); // Initialize system calls (syscall/sysret).

	printf("Jumping to user app!\n\n");
	user_jump((void *)user_app_virt_addr); // Just to user app in virtual space.

	/* Never exit! */
	while (1)
	{
	};
}


/*
 * Initialize 4-level page table to map top 1gb memory for the user-space and reuse bottom 4gb of kernel-space.
 * The only caveat is if user_stack + user_app size is > 2mb it will fail as we will need more than 512 ptes.
 * But for the purpose of the assignment, the logic works.
 */
uintptr_t page_table_init_user(information info, uintptr_t k_pml4e)
{
	void *user_pt_base = (void *)info.user_pt_base;
	uint64_t *kernel_pml4e = (uint64_t *)k_pml4e;

	uint64_t *u_pte = (uint64_t *)user_pt_base;
	for (unsigned int i = 0; i < info.num_user_stack_pages; i++)
	{
		u_pte[i] = (0x1000 * i) + (info.user_stack_buffer - (info.num_user_stack_pages * 0x1000)) + 0x7;
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

	uint64_t *u_pde = (uint64_t *)(u_pte + 512);
	uint64_t page_addr;
	for (int j = 0; j < info.num_user_pdes; j++)
	{
		uint64_t *pte_start = u_pte + 512 * j;
		page_addr = (uint64_t)pte_start;
		u_pde[j] = page_addr + 0x7;
	}
	for (int j = info.num_user_pdes; j < 512; j++) // Assuming num_user_pdes will be lesser than 512.
	{
		u_pde[j] = 0x0ULL;
	}

	uint64_t *u_pdpe = (uint64_t *)(u_pde + 512);
	for (int k = 0; k < 512 - info.num_user_pdpes; k++)
	{
		u_pdpe[k] = 0x0ULL;
	}
	u_pdpe[511] = (uint64_t)(u_pde) + 0x7; // Topmost entry here corresponds to last 1gb in virtual address space.

	uint64_t *u_pml4e = (uint64_t *)(u_pdpe + 512);
	u_pml4e[0] = *kernel_pml4e;
	for(int m = 1; m< 512 - info.num_user_pml4es; m++)
	{
		u_pml4e[m] = 0x0ULL;
	}
	u_pml4e[511] = (uint64_t)(u_pdpe) + 0x7; // Topmost entry here corresponds to last 1gb in virtual address space.

	return ((uintptr_t)u_pml4e);
}

/* 
 * Initialize 4-level page table to map 4gb memory for the kernel-space.
 */
uintptr_t page_table_init_kernel(information info)
{
	void *kernel_pt_base = (void *)info.kernel_pt_base;
	unsigned int num_k_pte = 1048576;		   //hardcode for 4gb mapping with 4kB page size.
	unsigned int num_k_pde = num_k_pte / 512;  // 2048
	unsigned int num_k_pdpe = num_k_pde / 512; // 4
	unsigned int num_pml4 = num_k_pdpe / 4;	   // 1

	uint64_t *k_pte = (uint64_t *)kernel_pt_base;
	for (unsigned int i = 0; i < num_k_pte; i++)
	{
		k_pte[i] = ((0x1000 * i) + 0x3);
	}

	uint64_t *k_pde = (uint64_t *)(k_pte + num_k_pte);
	uint64_t page_addr;
	for (int j = 0; j < num_k_pde; j++)
	{
		uint64_t *pte_start = k_pte + 512 * j;
		page_addr = (uint64_t)pte_start;
		k_pde[j] = page_addr + 0x3;
	}

	uint64_t *k_pdpe = (uint64_t *)(k_pde + num_k_pde);
	for (int k = 0; k < num_k_pdpe; k++)
	{
		uint64_t *pde_start = k_pde + 512 * k;
		page_addr = (uint64_t)pde_start;
		k_pdpe[k] = page_addr + 0x3;
	}
	for (int k = num_k_pdpe; k < 512; k++)
	{
		k_pdpe[k] = 0x0ULL;
	}

	uint64_t *pml4e = (uint64_t *)(k_pdpe + 512);
	for (int m = 0; m < num_pml4; m++)
	{
		uint64_t *pdpe_start = k_pdpe + 512 * m;
		page_addr = (uint64_t)pdpe_start;
		pml4e[m] = page_addr + 0x3;
	}
	for (int m = num_pml4; m < 511; m++)
	{
		pml4e[m] = 0x0ULL;
	}

	return ((uintptr_t)pml4e);
}

// Write value to the cr3 register.
void write_cr3(uintptr_t cr3_value)
{
	asm volatile("mov %0, %%cr3" ::"r"(cr3_value)
				 : "memory");
}