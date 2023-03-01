#include "portio.c"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"
#include "gdt.c"
#include "lib-header/framebuffer.h"
// #include "kernel_loader.s"
#include "lib-header/kernel_loader.h"


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