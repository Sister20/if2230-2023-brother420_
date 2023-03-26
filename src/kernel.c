#include "lib-header/portio.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"
#include "lib-header/gdt.h"
#include "lib-header/framebuffer.h"
#include "lib-header/kernel_loader.h"
#include "lib-header/idt.h"
#include "lib-header/interrupt.h"

void write_splash_screen3();

void kernel_setup(void) {
    enter_protected_mode(&_gdt_gdtr);
    pic_remap();
    initialize_idt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);
    while (TRUE) 
      keyboard_state_activate();
}


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
