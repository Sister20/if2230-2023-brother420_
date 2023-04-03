
#include "lib-header/paging.h"

__attribute__((aligned(0x1000))) struct PageDirectory _paging_kernel_page_directory = {
    .table = {
        [0] = {
            .flag.present_bit       = 1,
            .flag.write_bit         = 1,
            .lower_address          = 0,
            .flag.use_pagesize_4_mb = 1,
        },
        [0x300] = {
            .flag.present_bit       = 1,
            .flag.write_bit         = 1,
            .lower_address          = 0,
            .flag.use_pagesize_4_mb = 1,
        },
    }
};

static struct PageDriverState page_driver_state = {
    .last_available_physical_addr = (uint8_t*) 0 + PAGE_FRAME_SIZE,
};

void update_page_directory_entry(void *physical_addr, void *virtual_addr, struct PageDirectoryEntryFlag flag) {
    uint32_t page_index = ((uint32_t) virtual_addr >> 22) & 0x3FF;

    _paging_kernel_page_directory.table[page_index].flag          = flag;
    _paging_kernel_page_directory.table[page_index].lower_address = ((uint32_t)physical_addr >> 22) & 0x3FF;
    flush_single_tlb(virtual_addr);
}

/* Curiga salah di sini, help T_T BOOTLOOP */
int8_t allocate_single_user_page_frame(void *virtual_addr) {
    // Using default QEMU config (128 MiB max memory)
    uint32_t last_physical_addr = (uint32_t) page_driver_state.last_available_physical_addr;
    
    // TODO : Allocate Page Directory Entry with user privilege

    struct PageDirectoryEntry page_dir_entry = {
        .flag.present_bit           = 1,
        .flag.write_bit             = 1,
        .flag.userOrSupervisor      = 1,
        .flag.PWT                   = 0,
        .flag.PCD                   = 0,
        .flag.accessed_bit          = 0,
        .flag.dirty_bit             = 0,
        .flag.use_pagesize_4_mb     = 1,
        .lower_address              = last_physical_addr >> 22 & 0x3FF
    };
    
    if (last_physical_addr >= PAGE_FRAME_SIZE) {
        return -1;
    } else {
        // Update page directory entry
        update_page_directory_entry(&last_physical_addr, virtual_addr, page_dir_entry.flag);
        page_driver_state.last_available_physical_addr += PAGE_FRAME_SIZE;
        return 0;
    }
    
    return -1;
}


void flush_single_tlb(void *virtual_addr) {
    asm volatile("invlpg (%0)" : /* <Empty> */ : "b"(virtual_addr): "memory");
}