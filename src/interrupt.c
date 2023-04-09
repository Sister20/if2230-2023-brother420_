#include "lib-header/interrupt.h"
#include "lib-header/framebuffer.h"
#include "lib-header/gdt.h"
#include "lib-header/stdmem.h"
#include "lib-header/idt.h"

uint8_t row_shell = 0;
int current_directory_cluster = ROOT_CLUSTER_NUMBER;

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
void puts(char *str, uint32_t len, uint32_t color) {
    for (uint32_t i = 0; i < len; i++) {
        framebuffer_write(row_shell, i, str[i], color, 0);
    }
}

void puts2(char *str, uint32_t len, uint32_t color) {
    for (uint32_t i = 0; i < len; i++) {
        framebuffer_write(24, i, str[i], color, 0);
    }
}

void template(void){
    puts("Brother420/", 11, 0x0a);
    framebuffer_write(row_shell, 11, ':', 0x01, 0);
    framebuffer_write(row_shell, 12, '$', 0x0F, 0);
    framebuffer_set_cursor(row_shell, 14);
    row_shell++;
}

/**
 * cd		- Mengganti current working directory (termasuk .. untuk naik)
 * ls		- Menuliskan isi current working directory
 * mkdir	- Membuat sebuah folder kosong baru
 * cat		- Menuliskan sebuah file sebagai text file ke layar (Gunakan format LF newline)
 * cp		- Mengcopy suatu file
 * rm		- Menghapus suatu file atau folder kosong
 * mv		- Memindah dan merename lokasi file/folder
 * whereis	- Mencari file/folder dengan nama yang sama diseluruh file system

 * @return 0 cd (spasi)
 * @return 1 ls
 * @return 2 mkdir (spasi)
 * @return 3 cat (spasi)
 * @return 4 cp (spasi)
 * @return 5 rm (spasi)
 * @return 6 mv (spasi)
 * @return 7 whereis (spasi)
*/
uint8_t getCommandInput(char *input, uint8_t len){
    char *command[] = {"cd ", "ls", "mkdir ", "cat ", "cp ", "rm ", "mv ", "whereis "};
    uint8_t command_len[] = {3, 2, 6, 4, 3, 3, 3, 8};
    for (uint8_t i = 0; i < 8; i++){
        if (len >= command_len[i] && memcmp(input, command[i], command_len[i]) == 0){
            return i;
        }
        framebuffer_write(23, i, input[i], 0x0f, 0);
    }
    return 255;
}

void puts_line(char *input, int row, int col, int length, int color){
    for (int i = 0; i < length; i++){
        framebuffer_write(row, col + i, input[i], color, 0);
    }
}


void command_call_ls(void){
    struct FAT32DriverState state_driver;
    read_clusters(&state_driver.dir_table_buf, current_directory_cluster, 1);
    read_clusters(&state_driver.fat_table, 1, 1);
    
    int col = 0;
    int row_add_needed = 1;

    for (int i = 1; i < 64; i++){

        if (state_driver.dir_table_buf.table[i].user_attribute == UATTR_NOT_EMPTY){
            if (col == 5){
                col = 0;
                row_shell++;
                row_add_needed++;
            }
            puts_line(state_driver.dir_table_buf.table[i].name, row_shell, col*16, 8, 0x0f);
            col++;
        }
    }
    row_shell++;

    addRow(row_add_needed);

}

void command_call_mkdir(char *dirCommandName){
    
    struct FAT32DriverRequest request = {
        .buf                    = 0,
        .ext                    = "\0\0\0",
        .parent_cluster_number  = current_directory_cluster,
        .buffer_size            = 0,
    };
    for (int i = 6; i < 14; i++){
        request.name[i-6] = dirCommandName[i];
    }
    write(request);
}

void syscall(struct CPURegister cpu, __attribute__((unused)) struct InterruptStack info) {
    if (cpu.eax == 0) {
        struct FAT32DriverRequest request = *(struct FAT32DriverRequest*) cpu.ebx;
        *((int8_t*) cpu.ecx) = read(request);
        // if (*((int8_t*) cpu.ecx) == 0){
        //     puts(request.buf, request.buffer_size, 0x0e);
        // }
    } else if (cpu.eax == 4) {
        template();
        keyboard_state_activate();
        __asm__("sti"); // Due IRQ is disabled when main_interrupt_handler() called
        while (is_keyboard_blocking());
        char buf[KEYBOARD_BUFFER_SIZE];
        get_keyboard_buffer(buf);
        memcpy((char *) cpu.ebx, buf, cpu.ecx);
    } else if (cpu.eax == 5) {
        // ini berarti di-enter dari syscall 4
        
        uint8_t command = getCommandInput((char *) cpu.ebx, cpu.ecx);
        
        switch (command){
            case 0:
                // cd
                framebuffer_write(0, 70, '0', 0x0f, 0);
                break;
            case 1:
                // ls
                framebuffer_write(0, 70, '1', 0x0f, 0);
                command_call_ls();
                break;
            case 2:
                // mkdir
                framebuffer_write(0, 70, '2', 0x0f, 0);
                command_call_mkdir((char *) cpu.ebx);
                break;
            case 3:
                // cat
                framebuffer_write(0, 70, '3', 0x0f, 0);
                break;
            case 4:
                // cp
                framebuffer_write(0, 70, '4', 0x0f, 0);
                break;
            case 5:
                // rm
                framebuffer_write(0, 70, '5', 0x0f, 0);
                break;
            case 6:
                // mv
                framebuffer_write(0, 70, '6', 0x0f, 0);
                break;
            case 7:
                // whereis
                framebuffer_write(0, 70, '7', 0x0f, 0);
                break;
            default:
                // command not found
                framebuffer_write(0, 70, 'x', 0x0f, 0);
                break;
        }
            

        puts2((char *) cpu.ebx, cpu.ecx, cpu.edx); // Modified puts() on kernel side
    }
}


void main_interrupt_handler(struct CPURegister cpu, uint32_t int_number, struct InterruptStack info) {    switch (int_number) {

        case PIC1_OFFSET + IRQ_KEYBOARD:
            keyboard_isr();
            break;
        case 0x30:
            syscall(cpu, info);
            break;
    }
}

