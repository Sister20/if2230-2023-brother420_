#include "lib-header/interrupt.h"
#include "lib-header/framebuffer.h"
#include "lib-header/gdt.h"
#include "lib-header/stdmem.h"
#include "lib-header/idt.h"
#include "lib-header/currentdirectorystack.h"
#include "lib-header/directorystack.h"

uint8_t row_shell = 0;
int current_directory_cluster = ROOT_CLUSTER_NUMBER;
struct CURRENT_DIR_STACK current_dir_stack;
bool initialized_cd_stack = FALSE;

struct TSSEntry _interrupt_tss_entry = {
    .prev_tss = 0,
    .esp0 = 0,
    .ss0 = GDT_KERNEL_DATA_SEGMENT_SELECTOR,
    .unused_register = {0},
};


void io_wait(void) {
    out(0x80, 0);
}

void pic_ack(uint8_t irq) {
    if (irq >= 8)
        out(PIC2_COMMAND, PIC_ACK);
    out(PIC1_COMMAND, PIC_ACK);
}

void pic_remap(void) {
    uint8_t a1, a2;

    // Save masks
    a1 = in(PIC1_DATA); 
    a2 = in(PIC2_DATA);

    // Starts the initialization sequence in cascade mode
    out(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4); 
    io_wait();
    out(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    out(PIC1_DATA, PIC1_OFFSET); // ICW2: Master PIC vector offset
    io_wait();
    out(PIC2_DATA, PIC2_OFFSET); // ICW2: Slave PIC vector offset
    io_wait();
    out(PIC1_DATA, 0b0100);      // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
    io_wait();
    out(PIC2_DATA, 0b0010);      // ICW3: tell Slave PIC its cascade identity (0000 0010)
    io_wait();

    out(PIC1_DATA, ICW4_8086);
    io_wait();
    out(PIC2_DATA, ICW4_8086);
    io_wait();

    // Restore masks
    out(PIC1_DATA, a1);
    out(PIC2_DATA, a2);
}

// void main_interrupt_handler(
//     __attribute__((unused)) struct CPURegister cpu,
//     uint32_t int_number,
//     __attribute__((unused)) struct InterruptStack info
// ) {

//    switch (int_number) {
//         case PAGE_FAULT:
//             __asm__("hlt");
//             break;
//         case PIC1_OFFSET + IRQ_KEYBOARD:
//             keyboard_isr();
//             break;
//     }
// }

void activate_keyboard_interrupt(void) {
    out(PIC1_DATA, PIC_DISABLE_ALL_MASK ^ (1 << IRQ_KEYBOARD));
    out(PIC2_DATA, PIC_DISABLE_ALL_MASK);
}

void set_tss_kernel_current_stack(void) {
    uint32_t stack_ptr;
    // Reading base stack frame instead esp
    __asm__ volatile ("mov %%ebp, %0": "=r"(stack_ptr) : /* <Empty> */);
    // Add 8 because 4 for ret address and other 4 is for stack_ptr variable
    _interrupt_tss_entry.esp0 = stack_ptr + 8; 
}

// struct TSSEntry _interrupt_tss_entry = {
//     .ss0  = GDT_KERNEL_DATA_SEGMENT_SELECTOR,
// };


// TODO: implement puts using framebuffer
void puts(char *str, uint32_t len, uint32_t color, uint32_t row_shells) {
    for (uint32_t i = 0; i < len; i++) {
        framebuffer_write(row_shells, i, str[i], color, 0);
    }
}

/**
 * @param str string to be printed
 * @param len length of string
 * @param color color of string
*/
void puts2(char *str, uint32_t len, uint32_t color) {
    for (uint32_t i = 0; i < len; i++) {
        framebuffer_write(24, i, str[i], color, 0);
    }
}

void puts_long_text(char *str, uint32_t len, uint32_t color, uint8_t *row_shells) {
    uint8_t j = 0;
    uint8_t k = 0;
    for (uint32_t i = 0; i < len; i++) {
        if (j == 80) {
            (*row_shells)++;
            j = 0;
            k++;
        }
        if (str[i] == '\n') {
            (*row_shells)++;
            j = 0;
            k++;
            continue;
        }
        framebuffer_write(*row_shells, j, str[i], color, 0);
        j++;
    }
    addRow(k);
}

void template(uint32_t row_shells, char * cwd){
    uint8_t tempCol = 10;
    puts("brother420", 10, 0x0a, row_shells);
    while (cwd[tempCol - 10] != '\0'){
        framebuffer_write(row_shells, tempCol, cwd[tempCol - 10], 0x0a, 0);
        tempCol++;
    }
    framebuffer_write(row_shells, tempCol, '/', 0x01, 0);
    framebuffer_write(row_shells, tempCol+1, ':', 0x01, 0);
    framebuffer_write(row_shells, tempCol+2, '$', 0x0F, 0);
    framebuffer_set_cursor(row_shells, tempCol+4);
    setCol(tempCol+4);
}

void puts_line(char *input, int row, int col, int length, int color){
    for (int i = 0; i < length; i++){
        framebuffer_write(row, col + i, input[i], color, 0);
    }
}

void showWhereIs(struct DIR_STACK dir_stack, uint32_t row_shells){
    for (int i = 0; i < dir_stack.top; i++){
        puts((char *) &(dir_stack.stack[i]), 80, 0x0a, row_shells);
    }
}

void syscall(struct CPURegister cpu, __attribute__((unused)) struct InterruptStack info) {
    if (cpu.eax == 0) {
        struct FAT32DriverRequest request = *(struct FAT32DriverRequest*) cpu.ebx;
        *((int8_t*) cpu.ecx) = read(request);
        // if (*((int8_t*) cpu.ecx) == 0){
        //     puts(request.buf, request.buffer_size, 0x0e);
        // }
    } else if (cpu.eax == 1){
        // struct FAT32DriverRequest request = *(struct FAT32DriverRequest*) cpu.ebx;
        

    } else if (cpu.eax == 2) {
        struct FAT32DriverRequest request = *(struct FAT32DriverRequest*) cpu.ebx;
        *((int8_t*) cpu.ecx) = write(request);

    } else if (cpu.eax == 3) {
        struct FAT32DriverRequest request = *(struct FAT32DriverRequest*) cpu.ebx;
        *((int8_t*) cpu.ecx) = delete(request);

    } else if (cpu.eax == 4) {
        // char *cwd = convert_to_paths((struct CURRENT_DIR_STACK *) cpu.ecx);
        template((uint32_t) cpu.edx, (char *) cpu.ecx);
        clear_keyboard_buffer();
        keyboard_state_activate();
        __asm__("sti"); // Due IRQ is disabled when main_interrupt_handler() called
        while (is_keyboard_blocking());
        char buf[KEYBOARD_BUFFER_SIZE];
        get_keyboard_buffer(buf);
        memcpy((char *) cpu.ebx, buf, 0x100);

    } else if (cpu.eax == 5) {
        puts2((char *) cpu.ebx, cpu.ecx, cpu.edx); // Modified puts() on kernel side
    }

    // Syscall cd
    else if (cpu.eax == 6) {
        puts_line((char*) cpu.ebx, cpu.ecx, cpu.edx, 8, 0x0F);
    }

    // Syscall addrow
    else if (cpu.eax == 7) {
        addRow(cpu.ebx);
    }

    // Syscall read cluster
    else if (cpu.eax == 8) {
        read_clusters((struct FAT32DirectoryTable *) cpu.ebx, cpu.ecx, cpu.edx);
    }

    // Syscall init current directory
    else if (cpu.eax == 9) {
        init_current_dir_stack((struct CURRENT_DIR_STACK *) cpu.ebx);
    }

    // Syscall pop current dir
    else if (cpu.eax == 10) {
        pop_current_dir((struct CURRENT_DIR_STACK *) cpu.ebx);
        *((int*) cpu.ecx) = get_top_current_dir((struct CURRENT_DIR_STACK *) cpu.ebx);
    }

    // Syscall push current dir
    else if (cpu.eax == 11) {
        push_current_dir((struct CURRENT_DIR_STACK *) cpu.ebx, cpu.ecx, (char *) cpu.edx);
    }

    // Syscall puts long text
    else if (cpu.eax == 12) {
        puts_long_text((char *) cpu.ebx, cpu.ecx, 0x0E, (uint8_t*) (cpu.edx));
    }

    // Syscall write cluster
    else if (cpu.eax == 13) {
        write_clusters((struct FAT32DirectoryTable *) cpu.ebx, cpu.ecx, cpu.edx);
    }

    // Syscall get where is location
    else if (cpu.eax == 14) {
        push_dir((struct DIR_STACK * ) cpu.ebx, (char *) cpu.ecx);
    }

    // Syscall to convert to path
    else if (cpu.eax == 15) {
        char *cwd = convert_to_paths((struct CURRENT_DIR_STACK *) cpu.ebx);
        memcpy((char *) cpu.ecx, cwd, 0x100);
    }

    // Syscall initialize dir stack
    else if (cpu.eax == 16) {
        init_dir_stack((struct DIR_STACK *) cpu.ebx);
    }
}


void main_interrupt_handler(struct CPURegister cpu, uint32_t int_number, struct InterruptStack info) {    
    
    switch (int_number) {

        case PIC1_OFFSET + IRQ_KEYBOARD:
            keyboard_isr();
            break;
        case 0x30:
            syscall(cpu, info);
            break;

    };
}

