#include <kernel_syscall.h>
#include <types.h>
#include <msr.h>
#include <printf.h>

void *kernel_stack; /* Initialized in kernel_entry.S */
void *user_stack = NULL; /* Initialized in kernel.c */

void *syscall_entry_ptr; /* Points to syscall_entry(), initialized in kernel_entry.S; use that rather than syscall_entry() when obtaining its address */

long do_syscall_entry(long n, long a1, long a2, long a3, long a4, long a5)
{
	if (n == 0) // Handle only 0 for now which means write in our case.
	{
		printf((char *)a1);
	}
	else if (n == 1)
	{
		printf("The passed variable has the following value: %d \n", a1);
	}
	else
	{
		return -1;
	}
	return 0; /* Success */
}

void syscall_init(void)
{
	/* Enable SYSCALL/SYSRET */
	wrmsr(MSR_EFER, rdmsr(MSR_EFER) | 0x1);

	/* GDT descriptors for SYSCALL/SYSRET (USER descriptors are implicit) */
	wrmsr(MSR_STAR, ((uint64_t)GDT_KERNEL_DATA << 48) | ((uint64_t)GDT_KERNEL_CODE << 32));

	/* register a system call entry point */
	wrmsr(MSR_LSTAR, (uint64_t)syscall_entry_ptr);

	/* Disable interrupts (IF) while in a syscall */
	wrmsr(MSR_SFMASK, 1U << 9);
}
