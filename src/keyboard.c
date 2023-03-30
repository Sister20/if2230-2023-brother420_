#include "lib-header/keyboard.h"
#include "lib-header/portio.h"
#include "lib-header/stdmem.h"
#include "lib-header/framebuffer.h"
static struct KeyboardDriverState keyboard_state;
static int row = 0;
static char antiDouble = -1;
static int holding = 0;
static char charHold = -1;
static bool altFour = FALSE;

static int backspaceLine[25] = {
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0,
    0,0,0,0,0
};
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
      0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
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

bool altF4(){
  return altFour;
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
    bool capslock = FALSE;
    bool shifted = FALSE;
    int16_t holdWait = 0;
    if (!keyboard_state.keyboard_input_on){
        keyboard_state.buffer_index = 0;
        framebuffer_write(21,11,'L',0xa,0);
    }
    else {
      while (is_keyboard_blocking()){
        uint8_t  scancode    = in(KEYBOARD_DATA_PORT);
        char     mapped_char = keyboard_scancode_1_to_ascii_map[scancode];

        if (mapped_char != antiDouble){
          antiDouble = mapped_char;

          /* Backspace */
          if (mapped_char == '\b') {
            if (keyboard_state.buffer_index) {
              keyboard_state.buffer_index--;
              backspaceLine[row]--;
            } else if (keyboard_state.buffer_index == 0){
              if (row <= 0) {
                row = 0;
              } else {
                row--;
                keyboard_state.buffer_index = backspaceLine[row];
              }
            }
            framebuffer_write(row, keyboard_state.buffer_index, 0, 0x0c, 0);
            framebuffer_set_cursor(row, keyboard_state.buffer_index);
          } 
          
          /* Enter (deactivate keyboard) */
          else if (mapped_char=='\n') {
            keyboard_state.buffer_index=0;
            keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = 0;
            framebuffer_set_cursor(row + 1, keyboard_state.buffer_index);
            mapped_char = 0;
            if (row >= 24) {
              row = 24;
            } else {
              row++;
            }
            keyboard_state_deactivate();
          } 

          /* Up Arrow (no holding) */
          else if (scancode == 0x48){
            if (row > 0){
              row--;
              framebuffer_set_cursor(row, keyboard_state.buffer_index);
              do {
                scancode = in(KEYBOARD_DATA_PORT);
              } while (scancode != 0xc8);
            }
          } 
          
          /* Down Arrow (no holding) */
          else if (scancode == 0x50){
            if (row < 24){
              row++;
              framebuffer_set_cursor(row, keyboard_state.buffer_index);
              do {
                scancode = in(KEYBOARD_DATA_PORT);
              } while (scancode != 0xd0);
            }
          } 
          
          /* Left Arrow (no holding) */
          else if (scancode == 0x4b){
            if (keyboard_state.buffer_index > 0){
              keyboard_state.buffer_index--;
              framebuffer_set_cursor(row, keyboard_state.buffer_index);
              do {
                scancode = in(KEYBOARD_DATA_PORT);
              } while (scancode != 0xcb);
            }
          } 
          
          /* Right Arrow (no holding) */
          else if (scancode == 0x4d){
            if (keyboard_state.buffer_index < 79){
              keyboard_state.buffer_index++;
              framebuffer_set_cursor(row, keyboard_state.buffer_index);
              do {
                scancode = in(KEYBOARD_DATA_PORT);
              } while (scancode != 0xcd);
            }
          } 
          
          /* Caps Lock */
          else if (scancode == 0x3A){
            do {
              scancode = in(KEYBOARD_DATA_PORT);
            } while (scancode != 0xBA);
            capslock = !capslock;
          } 
          
          /* Shift */
          else if (scancode == 0x2A || scancode == 0x36){
            do {
              scancode = in(KEYBOARD_DATA_PORT);
            } while (scancode == 0x2A || scancode == 0x36);

            if (scancode == 0xAA || scancode == 0xB6){
              continue;
            } else {
              shifted = TRUE;
              mapped_char = keyboard_scancode_1_to_ascii_map[scancode];
            }
          } 
          
          /* Release shift */
          else if (scancode == 0xAA || scancode == 0xB6){ 
            shifted = FALSE;
          } 
          
          /* ctrl */
          else if (scancode == 0x1D){  // ctrl
            do {
              scancode = in(KEYBOARD_DATA_PORT);
            } while (scancode == 0x1D);
            
            scancode = in(KEYBOARD_DATA_PORT);
            /* Ctrl Home */
            if (scancode == 0x47){
              framebuffer_set_cursor(0, 0);
              keyboard_state.buffer_index = 0;
              row = 0;
            } 
            
            /* Ctrl End */
            else if (scancode == 0x4F){
              framebuffer_set_cursor(24, 79);
              keyboard_state.buffer_index = 79;
              row = 24;
            }
          } 

          /* Alt */
          else if (scancode == 0x38){ // alt
            do {
              scancode = in(KEYBOARD_DATA_PORT);
            } while (scancode == 0x38);
             
            /* alt f4 */ 
            if (scancode == 0x3E){
              altFour = TRUE;
            } 
          } 
          
          /* Home */
          else if (scancode == 0x47){
            framebuffer_set_cursor(row, 0);
            keyboard_state.buffer_index = 0;
            do {
              scancode = in(KEYBOARD_DATA_PORT);
            } while (scancode == 0x47);
          } 
          
          /* End */
          else if (scancode == 0x4F){ 
            framebuffer_set_cursor(row, 79);
            keyboard_state.buffer_index = 79;
            do {
              scancode = in(KEYBOARD_DATA_PORT);
            } while (scancode == 0x4F);
          }

          /* Normal Write */
          else if (mapped_char !=0){
            if (capslock && !shifted){ // hanya capslock
              if (mapped_char >= 'a' && mapped_char <= 'z'){ 
                mapped_char -= 32;
              }
            } else if (!capslock && shifted){ // hanya shift
              if (mapped_char >= 'a' && mapped_char <= 'z'){
                mapped_char -= 32;
              }
            } else if (capslock && shifted){ // capslock dan shift
              if (mapped_char >= 'A' && mapped_char <= 'Z'){
                mapped_char += 32;
              }
            }
            framebuffer_write(row, keyboard_state.buffer_index, mapped_char, 0x0c, 0);
            
            if (keyboard_state.buffer_index >= 79){
              if (row >= 24) {
                row = 24;
              } else {
                row++;
              }
              keyboard_state.buffer_index = 0;
            } else {
              keyboard_state.buffer_index++;
              backspaceLine[row]++;
              keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = mapped_char;
            }
            if (keyboard_state.buffer_index >= 79 && row < 24){
              framebuffer_set_cursor(row + 1, 0);
            } else if (row >= 24){ 
              framebuffer_set_cursor(24, 0);
            } else {
              framebuffer_set_cursor(row, keyboard_state.buffer_index);
            }
          } else {
            
          }

        } else {
          if (mapped_char == charHold){
            holding++;
          } else { 
            charHold = mapped_char;
            holding = 0;
          }

          if (holding == 0){
            holdWait = -5000;
          } else if (holding < 100) {
            holdWait = 4200;
          } else {
            holdWait = 5000;
          }

          antiDouble = -1;
          for (int i = 0; i < 600000 - (holdWait * 100); i++){
            io_wait();
          }
        }
      }
    }
    pic_ack(IRQ_KEYBOARD);
}
