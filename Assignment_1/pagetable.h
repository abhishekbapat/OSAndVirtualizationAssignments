#ifndef PAGETABLE_TYPEDEFS
#define PAGETABLE_TYPEDEFS
typedef unsigned long long u64;
#endif

#ifndef PAGETABLE_METHODS
#define PAGETABLE_METHODS
u64 page_table_init(void *);
void write_cr3(u64);
#endif