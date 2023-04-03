#include "lib-header/stdtype.h"
#include "lib-header/gdt.h"
#include "lib-header/interrupt.h"

/**
 * global_descriptor_table, predefined GDT.
 * Initial SegmentDescriptor already set properly according to GDT definition in Intel Manual & OSDev.
 * Table entry : [{Null Descriptor}, {Kernel Code}, {Kernel Data (variable, etc)}, ...].
 */


static struct GlobalDescriptorTable global_descriptor_table = {
    .table = {
        {/* TODO    : Null Descriptor */
            .segment_low        = 0,
            .base_low           = 0,
            .base_mid           = 0,
            .type_bit           = 0,
            .non_system         = 0,
            .DPL                = 0,
            .P                  = 0,
            .segment_limit      = 0,
            .AVL                = 0,
            .L                  = 0,
            .D_B                = 0,
            .G                  = 0,
            .base_high          = 0
        },
        {/* TODO    : Kernel Code Descriptor */
            .segment_low        = 0xFFFF,
            .base_low           = 0,
            .base_mid           = 0,
            .type_bit           = 0xA,
            .non_system         = 1,
            .DPL                = 0,
            .P                  = 1,
            .segment_limit      = 0xF,
            .AVL                = 0,
            .L                  = 0,
            .D_B                = 1,
            .G                  = 1,
            .base_high          = 0
        },
        {/* TODO    : Kernel Data Descriptor */
            .segment_low        = 0xFFFF,
            .base_low           = 0,
            .base_mid           = 0,
            .type_bit           = 0x2,
            .non_system         = 1,
            .DPL                = 0,
            .P                  = 1,
            .segment_limit      = 0xF,
            .AVL                = 0,
            .L                  = 0,
            .D_B                = 1,
            .G                  = 1,
            .base_high          = 0
        },
        {/* TODO    : User   Code Descriptor */
            .segment_low        = 0xFFFF,
            .base_low           = 0,
            .base_mid           = 0,
            .type_bit           = 0xA,
            .non_system         = 1,
            .DPL                = 0x3,    // User Descriptor Privilege Level
            .P                  = 1,
            .segment_limit      = 0xF,
            .AVL                = 0,
            .L                  = 0,
            .D_B                = 1,
            .G                  = 1,
            .base_high          = 0
        },
        {/* TODO    : User   Data Descriptor */
            .segment_low        = 0xFFFF,
            .base_low           = 0,
            .base_mid           = 0,
            .type_bit           = 0x2,
            .non_system         = 1,
            .DPL                = 0x3,    // User Descriptor Privilege Level
            .P                  = 1,
            .segment_limit      = 0xF,
            .AVL                = 0,
            .L                  = 0,
            .D_B                = 1,
            .G                  = 1,
            .base_high          = 0
        },
        {
            .segment_limit      = (sizeof(struct TSSEntry) & (0xF << 16)) >> 16,
            .segment_low        = sizeof(struct TSSEntry),
            .base_high          = 0,
            .base_mid           = 0,
            .base_low           = 0,
            .non_system         = 0,    // S bit
            .type_bit           = 0x9,
            .DPL                = 0,    // DPL
            .P                  = 1,    // P bit
            .D_B                = 1,    // D/B bit
            .L                  = 0,    // L bit
            .G                  = 0,    // G bit
        },
        {0}
    }
};


/**
 * _gdt_gdtr, predefined system GDTR. 
 * GDT pointed by this variable is already set to point global_descriptor_table above.
 * From: https://wiki.osdev.org/Global_Descriptor_Table, GDTR.size is GDT size minus 1.
 */
struct GDTR _gdt_gdtr = {
    
    // TODO : Implement, this GDTR will point to global_descriptor_table. 
    //        Use sizeof operator

    .size = sizeof(struct GlobalDescriptorTable) - 1,
    .address = &global_descriptor_table
    
};


void gdt_install_tss(void) {
    uint32_t base = (uint32_t) &_interrupt_tss_entry;
    global_descriptor_table.table[5].base_high = (base & (0xFF << 24)) >> 24;
    global_descriptor_table.table[5].base_mid  = (base & (0xFF << 16)) >> 16;
    global_descriptor_table.table[5].base_low  = base & 0xFFFF;
}
