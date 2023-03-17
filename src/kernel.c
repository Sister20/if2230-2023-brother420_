#include "lib-header/portio.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"
#include "lib-header/gdt.h"
#include "lib-header/framebuffer.h"
#include "lib-header/kernel_loader.h"

void write_splash_screen3();

void kernel_setup(void) {
    enter_protected_mode(&_gdt_gdtr);
    pic_remap();
    initialize_idt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);
    __asm__("int $0x4");
    while (TRUE);
}


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
