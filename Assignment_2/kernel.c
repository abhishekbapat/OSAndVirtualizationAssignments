/*
 * This kernel initializes a 4gb page table 1:1 mapping for the kernel space.
 * Then it maps the last 1gb for user space.
 */

#include <fb.h>
#include <printf.h>
#include <types.h>
#include <kernel_syscall.h>
#include <interrupts.h>
#include <apic.h>
#include <msr.h>

// Declare the methods.
uintptr_t page_table_init_kernel(information);
uintptr_t page_table_init_user(information, uintptr_t, uint8_t);
void write_cr3(uintptr_t);
void tss_segment_init(information);
void tls_init(information);

void *default_interrupt_handler_ptr;
void *page_fault_handler_ptr;

information global_info;
uintptr_t global_k_pml4e_base;

// Kernel entry point.
void kernel_start(void *kernel_stack_buffer, unsigned int *framebuffer, unsigned int width, unsigned int height, information *info)
{
	global_info = *info;

	fb_init(framebuffer, width, height);

	uintptr_t user_app_virt_addr = 0xFFFFFFFFC0001000;	 // Calculated manually according to the page table setup.
	uintptr_t user_stack_virt_addr = 0xFFFFFFFFC0001000; // Same as above because stack is mapped before user_app in virtual mem. As stack moves downwards we shift by 4096.
	user_stack = (void *)user_stack_virt_addr;			 // Initialize user stack.

	printf("Kernel Stack: %p\n", kernel_stack);
	printf("User Stack: %p\n", user_stack);

	printf("Initializing page tables for kernel and user space!\n");
	uintptr_t k_pml4e_base = page_table_init_kernel(*info);				   // Initialize kernel page tables.
	global_k_pml4e_base = k_pml4e_base;
	uintptr_t u_pml4e_base = page_table_init_user(*info, k_pml4e_base, 0); // Initialize user page tables.
	write_cr3(u_pml4e_base);											   // Pass the base pml4e to cr3.

	printf("Initializing system calls!\n");
	syscall_init(); // Initialize system calls (syscall/sysret).

	printf("Initialize task state segment for cpu 0!\n");
	tss_segment_init(*info); // Initialize task state segment.
	printf("TSS Stack: %p\n", (void *)info->tss_stack_buffer);

	printf("Initializing TLS!\n");
	tls_init(*info);

	printf("Initializing Interrupt Desciptor Table!\n");
	idt_init();

	x86_lapic_enable();

	printf("Jumping to user app!\n\n");
	user_jump((void *)user_app_virt_addr); // Just to user app in virtual space.

	/* Never exit! */
	while (1)
	{
	};
}

void tls_init(information info)
{
	tls_block_t *tls = (tls_block_t *)info.tls_buffer;
	__builtin_memset((void *)tls, 0x0, sizeof(tls_block_t));
	tls->myself = (uintptr_t)info.tls_buffer;
	wrmsr(MSR_FSBASE, (uint64_t)tls);
}

/*
 * Initializes the Interrupt Descriptor Table.
 */
void idt_init()
{
	idt_ptr.base = (uint64_t)idt;
	idt_ptr.limit = sizeof(idt_entry_t) * IDT_TABLE_SIZE - 1;

	for (int i = 0; i < NUM_CPU_EXCEPTIONS; i++) // Set the default handler pointer for the first 32 IDT entries.
	{
		set_idt_entry(i, (uint64_t)default_interrupt_handler_ptr, (uint16_t)0x8, (uint8_t)0x8E);
	}

	for (int i = NUM_CPU_EXCEPTIONS; i < IDT_TABLE_SIZE; i++) // Initialize all other IDT entries to point to addr 0x0ULL.
	{
		set_idt_entry(i, (uint64_t)0x0ULL, (uint16_t)0x8, (uint8_t)0x8E);
	}

	// Initialize page fault handler. Comment this line out to handle page fault with default handler (Q3.2).
	set_idt_entry(PAGE_FAULT_IDT_INDEX, (uint64_t)page_fault_handler_ptr, (uint16_t)0x8, (uint8_t)0x8E);

	// Initializes the 32nd IDT entry to the apic handler.
	set_idt_entry(APIC_INTERRUPT_ENTRY, (uint64_t)apic_handler_ptr, (uint16_t)0x8, (uint8_t)0x8E);

	load_idt(&idt_ptr);
}

void set_idt_entry(uint8_t entry_num, uint64_t addr, uint16_t selector, uint8_t type_attr)
{
	idt[entry_num].offset_1 = (uint16_t)addr & 0xFFFF;
	idt[entry_num].selector = selector;
	idt[entry_num].ist = 0;
	idt[entry_num].type_attr = type_attr;
	idt[entry_num].offset_2 = (uint16_t)((addr >> 16) & 0xFFFF);
	idt[entry_num].offset_3 = (uint32_t)((addr >> 32) & 0xFFFFFFFF);
	idt[entry_num].reserved = 0;
}

void default_interrupt_handler(uint64_t rsp)
{
	printf("An exception has occurred. %%rsp: %p\n", (void *)rsp);
}

void page_fault_handler()
{
	printf("Page fault has occurred. Allocating a page to the address.\n");
	uintptr_t u_pml4e_base = page_table_init_user(global_info, global_k_pml4e_base, 1); // Initialize user page tables with extra page.
	write_cr3(u_pml4e_base);	
}

/*
 * Sets the tss stack pointer rsp0.
 * Passes the gdt tss offset value for initialization. 
 */
void tss_segment_init(information info)
{
	uint16_t tss_offset = 0x28;
	tss_segment_t *tss_segment = (tss_segment_t *)info.tss_segment_buffer;
	__builtin_memset((void *)tss_segment, 0x0, sizeof(tss_segment_t));
	uint64_t tss_stack = (uint64_t)info.tss_stack_buffer;
	tss_segment->rsp[0] = tss_stack;
	tss_segment->iopb_base = sizeof(tss_segment_t);
	load_tss_segment(tss_offset, tss_segment);
}

/*
 * Initialize 4-level page table to map top 1gb memory for the user-space and reuse bottom 4gb of kernel-space.
 * The only caveat is if user_stack + user_app size is > 2mb it will fail as we will need more than 512 ptes.
 * But for the purpose of the assignment, the logic works.
 */
uintptr_t page_table_init_user(information info, uintptr_t k_pml4e, uint8_t redef)
{
	void *user_pt_base = (void *)info.user_pt_base;
	uint64_t *kernel_pml4e = (uint64_t *)k_pml4e;

	uint64_t *u_pte = (uint64_t *)user_pt_base;
	for (unsigned int i = 0; i < info.num_user_stack_pages; i++)
	{
		u_pte[i] = (uint64_t)((0x1000 * i) + (info.user_stack_buffer - (info.num_user_stack_pages * 0x1000)) + 0x7);
	}
	for (unsigned int i = info.num_user_stack_pages; i < info.num_user_ptes; i++)
	{
		unsigned int temp = i - info.num_user_stack_pages;
		u_pte[i] = (uint64_t)((0x1000 * temp) + info.user_app_buffer + 0x7);
	}
	for (unsigned int i = info.num_user_ptes; i < 512; i++) //Assuming num_user_ptes will be lesser than 512.
	{
		u_pte[i] = (uint64_t)0x0ULL;
	}
	if(redef == 1)
	{
		u_pte[511] = (uint64_t)(info.extra_page_for_exception + 0x7);
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
	for (int m = 1; m < 512 - info.num_user_pml4es; m++)
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