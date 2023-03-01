#include "lib-header/portio.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"
#include "lib-header/gdt.h"
#include "lib-header/framebuffer.h"
#include "lib-header/kernel_loader.h"

void write_splash_screen();
void kernel_setup(void) {
    uint32_t a;
    uint32_t volatile b = 0x0000BABE;
    __asm__("mov $0xCAFE0000, %0" : "=r"(a));
    enter_protected_mode(&_gdt_gdtr);
    framebuffer_clear();
    framebuffer_write(3, 8,  'H', 0, 0xF);
    framebuffer_write(3, 9,  'a', 0, 0xF);
    framebuffer_write(3, 10, 'i', 0, 0xF);
    framebuffer_write(3, 11, '!', 0, 0xF);
    write_splash_screen3();
    framebuffer_set_cursor(3, 9);
    while (TRUE);
    while (TRUE) b += 1;
}

void write_splash_screen() {
    framebuffer_write(10, 30, 'B', 2, 0);
    framebuffer_write(10, 31, 'R', 2, 0);
    framebuffer_write(10, 32, 'O', 2, 0);
    framebuffer_write(10, 33, 'T', 2, 0);
    framebuffer_write(10, 34, 'H', 2, 0);
    framebuffer_write(10, 35, 'E', 2, 0);
    framebuffer_write(10, 36, 'R', 2, 0);
    framebuffer_write(10, 37, '4', 2, 0);
    framebuffer_write(10, 38, '2', 2, 0);
    framebuffer_write(10, 39, '0', 2, 0);
}
// void write_splash_screen() {
//     framebuffer_write(10, 0, "BBBBB     RRRRRR      OOOOOO   TTTTTTTT  HH    HH  EEEEEEE  RRRRRR  ", 2, 0);
//     framebuffer_write(11, 0, "BB   BB   RR    RR   OO     OO    TT     HH    HH  EE       RR   RR ", 2, 0);
//     framebuffer_write(12, 0, "BBBBBB    RRRRRR     OO     OO    TT     HHHHHHHH  EEEEE    RRRRRR  ", 2, 0);
//     framebuffer_write(13, 0, "BB   BB   RR   RR    OO     OO    TT     HH    HH  EE       RR   RR ", 2, 0);
//     framebuffer_write(14, 0, "BBBBB     RR    RR    OOOOOO      TT     HH    HH  EEEEEEE  RR    RR", 2, 0);
//     framebuffer_write(15, 0, "            44       44  22222222222    0000000                     ", 2, 0);
//     framebuffer_write(16, 0, "            44       44  22       22  00       00                   ", 2, 0);
//     framebuffer_write(17, 0, "            44       44  22       22  00       00                   ", 2, 0);
//     framebuffer_write(18, 0, "            44       44           22  00       00                   ", 2, 0);
//     framebuffer_write(19, 0, "            44444444444  22222222222  00       00                   ", 2, 0);
//     framebuffer_write(20, 0, "                     44  22           00       00                   ", 2, 0);
//     framebuffer_write(21, 0, "                     44  22222222222    0000000                     ", 2, 0);
// }

void write_splash_screen3() {
    char splash_screen[] = "BBBBB     RRRRRR      OOOOOO   TTTTTTTT  HH    HH  EEEEEEE  RRRRRR  \n"
                           "BB   BB   RR    RR   OO     OO    TT     HH    HH  EE       RR   RR \n"
                           "BBBBBB    RRRRRR     OO     OO    TT     HHHHHHHH  EEEEE    RRRRRR  \n"
                           "BB   BB   RR   RR    OO     OO    TT     HH    HH  EE       RR   RR \n"
                           "BBBBB     RR    RR    OOOOOO      TT     HH    HH  EEEEEEE  RR    RR\n"
                           "            44       44  22222222222    0000000                     \n"
                           "            44       44  22       22  00       00                   \n"
                           "            44       44  22       22  00       00                   \n"
                           "            44       44           22  00       00                   \n"
                           "            44444444444  22222222222  00       00                   \n"
                           "                     44  22           00       00                   \n"
                           "                     44  22222222222    0000000                     \n";

    int row = 10, col = 30;
    for (int i = 0; i < strlen(splash_screen); i++) {
        if (splash_screen[i] == '\n') {
            row++;
            col = 30;
        } else {
            framebuffer_write(row, col, splash_screen[i], 2, 0);
            col++;
        }
    }
}
