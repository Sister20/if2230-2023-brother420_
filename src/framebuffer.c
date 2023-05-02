#include "lib-header/framebuffer.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"
#include "lib-header/portio.h"

uint8_t* mfb = MEMORY_FRAMEBUFFER;

void framebuffer_set_cursor(uint8_t r, uint8_t c) {
    uint16_t cursor_index = r * 80 + c;
    out(CURSOR_PORT_CMD, 0x0F);
    out(CURSOR_PORT_DATA, (uint8_t) (cursor_index & 0xFF));
    out(CURSOR_PORT_CMD, 0x0E);
    out(CURSOR_PORT_DATA, (uint8_t) ((cursor_index >> 8) & 0xFF));
    
    // TODO : Implement
}

void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg) {
    int frame_buffer_index = (row * 80 + col) * 2;
    mfb[frame_buffer_index] = c;
    mfb[frame_buffer_index + 1] = (bg << 4) | fg;       

    // TODO : Implement
}

void framebuffer_clear(void) {
    size_t mfb_size = 80 * 25 * 2;
    for (size_t i = 0; i < mfb_size; i++)
        if (i % 2 == 0){
            mfb[i] = 0x00;
        }
        else {
            mfb[i] = 0x07;
        }

    // TODO : Implement
}
