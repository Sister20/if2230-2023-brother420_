#include "lib-header/keyboard.h"
#include "lib-header/portio.h"
#include "lib-header/stdmem.h"
#include "lib-header/framebuffer.h"
static struct KeyboardDriverState keyboard_state;
static int row=0;
const char keyboard_scancode_1_to_ascii_map[256] = {
      0, 0x1B, '1', '2', '3', '4', '5', '6',  '7', '8', '9',  '0',  '-', '=', '\b', '\t',
    'q',  'w', 'e', 'r', 't', 'y', 'u', 'i',  'o', 'p', '[',  ']', '\n',   0,  'a',  's',
    'd',  'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0, '\\',  'z', 'x',  'c',  'v',
    'b',  'n', 'm', ',', '.', '/',   0, '*',    0, ' ',   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0, '-',    0,    0,   0,  '+',    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,

      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
};

/* -- Driver Interfaces -- */

// Activate keyboard ISR / start listen keyboard & save to buffer
void keyboard_state_activate(void){
  keyboard_state.keyboard_input_on=TRUE;
}

// Deactivate keyboard ISR / stop listening keyboard interrupt
void keyboard_state_deactivate(void){
  keyboard_state.keyboard_input_on=FALSE;
}

// Get keyboard buffer values - @param buf Pointer to char buffer, recommended size at least KEYBOARD_BUFFER_SIZE
void get_keyboard_buffer(char *buf){
  for (int i = 0; i < KEYBOARD_BUFFER_SIZE; i++) {
        buf[i] = keyboard_state.keyboard_buffer[i];
    }
}

// Check whether keyboard ISR is active or not - @return Equal with keyboard_input_on value
bool is_keyboard_blocking(void){
  return keyboard_state.keyboard_input_on;
}


/* -- Keyboard Interrupt Service Routine -- */

/**
 * Handling keyboard interrupt & process scancodes into ASCII character.
 * Will start listen and process keyboard scancode if keyboard_input_on.
 * 
 * Will only print printable character into framebuffer.
 * Stop processing when enter key (line feed) is pressed.
 * 
 * Note that, with keyboard interrupt & ISR, keyboard reading is non-blocking.
 * This can be made into blocking input with `while (is_keyboard_blocking());` 
 * after calling `keyboard_state_activate();`
 */
void keyboard_isr(void) {
  
    if (!keyboard_state.keyboard_input_on){
        keyboard_state.buffer_index = 0;
        framebuffer_write(21,11,'L',0xa,0);
    }
    else {
        uint8_t  scancode    = in(KEYBOARD_DATA_PORT);
        char     mapped_char = keyboard_scancode_1_to_ascii_map[scancode];
        if(mapped_char=='\b'){
          if(keyboard_state.buffer_index){
            keyboard_state.buffer_index--;
          } else if(keyboard_state.buffer_index==0){
            row--;
          }
          framebuffer_set_cursor(row, keyboard_state.buffer_index);
          mapped_char=0;
        } else if(mapped_char=='\n'){
          keyboard_state.buffer_index=0;
          keyboard_state.keyboard_buffer[keyboard_state.buffer_index]=0;
          framebuffer_set_cursor(row, keyboard_state.buffer_index);
          mapped_char=0;
          row++;
        } else if(mapped_char!=0 || mapped_char=='\n' || mapped_char=='\b'){
          keyboard_state.buffer_index++;
          keyboard_state.keyboard_buffer[keyboard_state.buffer_index]=mapped_char;
          framebuffer_set_cursor(row, keyboard_state.buffer_index);
        } else {

        }
        framebuffer_write(row, keyboard_state.buffer_index, mapped_char, 0x0c, 0);
    }
    pic_ack(IRQ_KEYBOARD);
}

