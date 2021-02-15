typedef unsigned long long u64;

struct page_table_entry
{
    u64 present:1; 
    u64 read_write:1;
    u64 user_supervisor:1;
    u64 page_level_writethough:1;
    u64 page_level_cache_disable:1;
    u64 access:1;
    u64 dirty:1;
    u64 pat:1;
    u64 global_page:1;
    u64 avl:3;
    u64 page_address:40;
    u64 avail:7;
    u64 pke:4;
    u64 nonexecute:1;
};

struct page_directory_entry
{
    u64 present:1; 
    u64 read_write:1;
    u64 user_supervisor:1;
    u64 page_level_writethough:1;
    u64 page_level_cache_disable:1;
    u64 access:1;
    u64 ign:3;
    u64 avl:3;
    u64 page_address:40;
    u64 avail:11;
    u64 nonexecute:1;
};

struct page_directory_pointer_entry
{
    u64 present:1; 
    u64 read_write:1;
    u64 user_supervisor:1;
    u64 page_level_writethough:1;
    u64 page_level_cache_disable:1;
    u64 access:1;
    u64 ign:3;
    u64 avl:3;
    u64 page_address:40;
    u64 avail:11;
    u64 nonexecute:1;
};

struct page_map_level4_entry
{
    u64 present:1; 
    u64 read_write:1;
    u64 user_supervisor:1;
    u64 page_level_writethough:1;
    u64 page_level_cache_disable:1;
    u64 access:1;
    u64 ign:1;
    u64 mbz:2;
    u64 avl:3;
    u64 page_address:40;
    u64 avail:11;
    u64 nonexecute:1;
};