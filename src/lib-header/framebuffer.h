#ifndef _FRAMEBUFFER_H
#define _FRAMEBUFFER_H


#include "stdtype.h"

#define MEMORY_FRAMEBUFFER (uint8_t *) 0xB8000
#define CURSOR_PORT_CMD    0x03D4
#define CURSOR_PORT_DATA   0x03D5

// uint8_t* mfb = MEMORY_FRAMEBUFFER;

/**
 * Terminal framebuffer
 * Resolution: 80x25
 * Starting at MEMORY_FRAMEBUFFER,
 * - Even number memory: Character, 8-bit
 * - Odd number memory:  Character color lower 4-bit, Background color upper 4-bit
*/

/**
 * Set framebuffer character and color with corresponding parameter values.
 * More details: https://en.wikipedia.org/wiki/BIOS_color_attributes
 *
 * @param row Vertical location (index start 0)
 * @param col Horizontal location (index start 0)
 * @param c   Character
 * @param fg  Foreground / Character color
 * @param bg  Background color
 */
void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg);
// {
//     int frame_buffer_index = (row * 80 + col) * 2;
//     mfb[frame_buffer_index] = c;
//     mfb[frame_buffer_index + 1] = (bg << 4) | fg;
// }

/**
 * Set cursor to specified location. Row and column starts from 0
 * 
 * @param r row
 * @param c column
*/
void framebuffer_set_cursor(uint8_t r, uint8_t c);
// {
//     uint16_t cursor_index = r * 80 + c;
//     out(CURSOR_PORT_CMD, 0x0F);
//     out(CURSOR_PORT_DATA, (uint8_t) (cursor_index & 0xFF));
//     out(CURSOR_PORT_CMD, 0x0E);
//     out(CURSOR_PORT_DATA, (uint8_t) ((cursor_index >> 8) & 0xFF));
// }

/** 
 * Set all cell in framebuffer character to 0x00 (empty character)
 * and color to 0x07 (gray character & black background)
 * 
 */
void framebuffer_clear(void);
// {
//     size_t mfb_size = 80 * 25 * 2;
//     for (size_t i = 0; i < mfb_size; i++)
//         if (i % 2 == 0){
//             mfb[i] = 0x00;
//         }
//         else {
//             mfb[i] = 0x07;
//         }
// }

#endif