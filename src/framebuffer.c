#include "lib-header/framebuffer.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"
#include "lib-header/portio.h"

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

void framebuffer_set_cursor(uint8_t r, uint8_t c) {
    // TODO : Implement
    uint16_t pos = r * 80 + c;
    outb(CURSOR_PORT_CMD, 0x0F);
    outb(CURSOR_PORT_DATA, (uint8_t) (pos & 0xFF));
    outb(CURSOR_PORT_CMD, 0x0E);
    outb(CURSOR_PORT_DATA, (uint8_t) ((pos >> 8) & 0xFF));
}

void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg) {
    // TODO : Implement
    uint16_t idx = (row * 80 + col) * 2;
    MEMORY_FRAMEBUFFER[idx] = c;
    MEMORY_FRAMEBUFFER[idx + 1] = ((bg << 4) | (fg & 0x0F));
}

void framebuffer_clear(void) {
    // TODO : Implement
    for (int i = 0; i < 80 * 25 * 2; i += 2) {
        MEMORY_FRAMEBUFFER[i] = ' ';
        MEMORY_FRAMEBUFFER[i + 1] = 0x07;
    }
}

// #include "lib-header/stdtype.h"
// #include "lib-header/framebuffer.h"

// #define FRAMEBUFFER_ROWS 25
// #define FRAMEBUFFER_COLS 80

// static uint16_t framebuffer_index(uint8_t row, uint8_t col) {
//     return 2 * (row * FRAMEBUFFER_COLS + col);
// }

// void framebuffer_set_cursor(uint8_t r, uint8_t c) {
//     // TODO : Implement
//     uint16_t index = r * FRAMEBUFFER_COLS + c;
//     outb(CURSOR_PORT_CMD, 0x0F);
//     outb(CURSOR_PORT_DATA, (uint8_t) (index & 0xFF));
//     outb(CURSOR_PORT_CMD, 0x0E);
//     outb(CURSOR_PORT_DATA, (uint8_t) ((index >> 8) & 0xFF));
// }

// void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg) {
//     // TODO : Implement
//     uint16_t index = framebuffer_index(row, col);
//     MEMORY_FRAMEBUFFER[index] = c;
//     MEMORY_FRAMEBUFFER[index + 1] = (bg << 4) | fg;
// }

// void framebuffer_clear(void) {
//     // TODO : Implement
//     for (uint8_t row = 0; row < FRAMEBUFFER_ROWS; row++) {
//         for (uint8_t col = 0; col < FRAMEBUFFER_COLS; col++) {
//             framebuffer_write(row, col, ' ', 0x07, 0x00);
//         }
//     }
//     framebuffer_set_cursor(0, 0);
// }

