// #include "lib-header/keyboard.h"
#include "lib-header/portio.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"
#include "lib-header/gdt.h"
#include "lib-header/framebuffer.h"
#include "lib-header/kernel_loader.h"
#include "lib-header/idt.h"
#include "lib-header/keyboard.h"
#include "lib-header/interrupt.h"
#include "lib-header/fat32.h"


void write_splash_screen3();

void kernel_setup(void) {
    enter_protected_mode(&_gdt_gdtr);
    keyboard_state_activate();
    pic_remap();
    initialize_idt();
    activate_keyboard_interrupt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);
    initialize_filesystem_fat32();
    while (TRUE)
        keyboard_state_activate();

    // struct ClusterBuffer cbuf[5];
    // for (uint32_t i = 0; i < 5; i++)
    //     for (uint32_t j = 0; j < CLUSTER_SIZE; j++)
    //         cbuf[i].buf[j] = i + 'a';

    // struct FAT32DriverRequest request = {
    //     .buf                   = cbuf,
    //     .name                  = "ikanaide",
    //     .ext                   = "uwu",
    //     .parent_cluster_number = ROOT_CLUSTER_NUMBER,
    //     .buffer_size           = 0,
    // } ;

    // write(request);  // Create folder "ikanaide"
    // memcpy(request.name, "kano1\0\0\0", 8);
    // write(request);  // Create folder "kano1"
    // memcpy(request.name, "ikanaide", 8);
    // delete(request); // Delete first folder, thus creating hole in FS

    // memcpy(request.name, "daijoubu", 8);
    // request.buffer_size = 5*CLUSTER_SIZE;
    // write(request);  // Create fragmented file "daijoubu"

    // struct ClusterBuffer readcbuf;
    // read_clusters(&readcbuf, ROOT_CLUSTER_NUMBER+1, 1); 
    // // If read properly, readcbuf should filled with 'a'

    // request.buffer_size = CLUSTER_SIZE;
    // read(request);   // Failed read due not enough buffer size
    // request.buffer_size = 5*CLUSTER_SIZE;
    // read(request);   // Success read on file "daijoubu"

    while (TRUE);
}


// void kernel_setup(void) {
//     enter_protected_mode(&_gdt_gdtr);
//     pic_remap();
//     initialize_idt();
//     framebuffer_clear();
//     framebuffer_set_cursor(0, 0);
//     while (TRUE) 
//       keyboard_state_activate();
// }


// void kernel_setup(void) {
//     enter_protected_mode(&_gdt_gdtr);
//     pic_remap();
//     initialize_idt();
//     framebuffer_clear();
//     write_splash_screen3();
//     framebuffer_set_cursor(0, 0);
//     __asm__("int $0x4");
//     while (TRUE);
    
// }


/* Cek rusak */
// void kernel_setup(void) {
//     uint32_t a;
//     uint32_t volatile b = 0x0000BABE;
//     __asm__("mov $0xCAFE0000, %0" : "=r"(a));
//     enter_protected_mode(&_gdt_gdtr);
//     framebuffer_clear();
//     write_splash_screen3();
//     framebuffer_set_cursor(9, 40);
//     while (TRUE);
//     while (TRUE) b += 1;
// }


void write_splash_screen3() {
    char splash_screen[] = "===============================================================================\n"
                           "||                                                                           ||\n"
                           "||                                                                           ||\n"
                           "||   BBBBB     RRRRRR      OOOOOO   TTTTTTTT  HH    HH  EEEEEEE  RRRRRR      ||\n"
                           "||   BB   BB   RR    RR   OO     OO    TT     HH    HH  EE       RR   RR     ||\n"
                           "||   BBBBBB    RRRRRR     OO     OO    TT     HHHHHHHH  EEEEE    RRRRRR      ||\n"
                           "||   BB   BB   RR   RR    OO     OO    TT     HH    HH  EE       RR   RR     ||\n"
                           "||   BBBBB     RR    RR    OOOOOO      TT     HH    HH  EEEEEEE  RR    RR    ||\n"
                           "||                                                                           ||\n"
                           "||               44       44  22222222222    0000000                         ||\n"
                           "||               44       44  22       22  00       00                       ||\n"
                           "||               44       44  22       22  00       00                       ||\n"
                           "||               44       44           22  00       00                       ||\n"
                           "||               44444444444  22222222222  00       00                       ||\n"
                           "||                        44  22           00       00                       ||\n"
                           "||                        44  22222222222    0000000                         ||\n"
                           "||                                                                           ||\n"
                           "||                                                                           ||\n"
                           "===============================================================================\n";

    int row = 2, col = 0;
    for (int i = 0; i < 80*19; i++) {
        if (splash_screen[i] == '\n') {
            row++;
            col = 0;
        } else {
            if (splash_screen[i] == '4')
                framebuffer_write(row, col, splash_screen[i], 0x0c, 0);
            else if (splash_screen[i] == '2')
                framebuffer_write(row, col, splash_screen[i], 0x0a, 0);
            else if (splash_screen[i] == '0')
                framebuffer_write(row, col, splash_screen[i], 0x09, 0);
            else
                framebuffer_write(row, col, splash_screen[i], 0x0b, 0);
            col++;
        }
    }
}
