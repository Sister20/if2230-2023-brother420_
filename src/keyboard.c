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
static bool restoreSplash = FALSE;

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

bool restoreSplashScreen(){
  return restoreSplash;
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
        restoreSplash = FALSE;

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

            /* Ctrl Enter (restore splash screen)*/
            else if (scancode == 0x1C){
              framebuffer_set_cursor(0, 0);
              keyboard_state.buffer_index = 0;
              row = 0;
              restoreSplash = TRUE;
              keyboard_state_deactivate();
            }

            /* Ctrl End */
            else if (scancode == 0x4F){
              framebuffer_set_cursor(24, 79);
              keyboard_state.buffer_index = 79;
              row = 24;
            }

            /* Ctrl Shift */
            else if (scancode == 0x2A || scancode == 0x36){
              do {
                scancode = in(KEYBOARD_DATA_PORT);
              } while (scancode == 0x2A || scancode == 0x36);

              if (scancode == 0xAA || scancode == 0xB6){
                continue;
                /* Ctrl Shift Esc */
              } else if (scancode == 0x01){
                framebuffer_write(24,0,'N',0x0c,0);
                framebuffer_write(24,1,'O',0x0c,0);
                framebuffer_write(24,3,'T',0x0c,0);
                framebuffer_write(24,4,'A',0x0c,0);
                framebuffer_write(24,5,'S',0x0c,0);
                framebuffer_write(24,6,'K',0x0c,0);
                framebuffer_write(24,8,'M',0x0c,0);
                framebuffer_write(24,9,'A',0x0c,0);
                framebuffer_write(24,10,'N',0x0c,0);
                framebuffer_write(24,11,'A',0x0c,0);
                framebuffer_write(24,12,'G',0x0c,0);
                framebuffer_write(24,13,'E',0x0c,0);
                framebuffer_write(24,14,'R',0x0c,0);
                framebuffer_write(24,16,'L',0x0d,0);
                framebuffer_write(24,17,'O',0x0d,0);
                framebuffer_write(24,18,'L',0x0d,0);
                do {
                  scancode = in(KEYBOARD_DATA_PORT);
                } while (scancode == 0x01);

                for (int i = 0; i < 19; i++){
                  framebuffer_write(24, i, ' ', 0x0c, 0);
                }
              }
            }

            /* Ctrl Alt Backspace */
            /* Karena Ctrl Alt Delete memengaruhi OS aslinya */
            else if (scancode == 0x38){
              do {
                scancode = in(KEYBOARD_DATA_PORT);
              } while (scancode == 0x38);
              framebuffer_write(24,2,mapped_char,0x0c,0);

              if (scancode == 0x0e){
                for (uint8_t i = 3; i < 20; i == 6 ? i += 10 : i++){
                  framebuffer_write(i,0,'S',0x0c,0);
                  framebuffer_write(i,1,'Y',0x0c,0);
                  framebuffer_write(i,2,'S',0x0c,0);
                  framebuffer_write(i,3,'T',0x0c,0);
                  framebuffer_write(i,4,'E',0x0c,0);
                  framebuffer_write(i,5,'M',0x0c,0);
                  framebuffer_write(i,6,' ',0x0c,0);
                  framebuffer_write(i,7,'R',0x0c,0);
                  framebuffer_write(i,8,'E',0x0c,0);
                  framebuffer_write(i,9,'S',0x0c,0);
                  framebuffer_write(i,10,'E',0x0c,0);
                  framebuffer_write(i,11,'T',0x0c,0);
                  framebuffer_write(i,12,'I',0x0c,0);
                  framebuffer_write(i,13,'N',0x0c,0);
                  framebuffer_write(i,14,'G',0x0c,0);
                  framebuffer_write(i,15,' ',0x0c,0);
                  framebuffer_write(i,16,'P',0x0c,0);
                  framebuffer_write(i,17,'R',0x0c,0);
                  framebuffer_write(i,18,'O',0x0c,0);
                  framebuffer_write(i,19,'G',0x0c,0);
                  framebuffer_write(i,20,'R',0x0c,0);
                  framebuffer_write(i,21,'A',0x0c,0);
                  framebuffer_write(i,22,'M',0x0c,0);
                  framebuffer_write(i,23,'.',0x0c,0);
                  framebuffer_write(i,24,'.',0x0c,0);
                  framebuffer_write(i,25,'.',0x0c,0);
                }
                for (uint8_t i = 0; i < 80; i++){

                  for (uint8_t j = 9; j < 14; j++){
                    framebuffer_write(j, i, '/', 0x0c, 0);
                  }

                  for (uint32_t j = 0; j < 120000; j++){
                    io_wait();
                  }

                  for (uint8_t j = 9; j < 14; j++){
                    framebuffer_write(j, i, '\\', 0x0c, 0x0);
                  }

                  for (uint32_t j = 0; j < 120000; j++){
                    io_wait();
                  }

                  for (uint8_t j = 9; j < 14; j++){
                    framebuffer_write(j, i, '-', 0x0c, 0x0);
                  }

                  for (uint32_t j = 0; j < 120000; j++){
                    io_wait();
                  }
                  if (i != 0){
                    for (uint8_t j = 9; j < 14; j++){
                      framebuffer_write(j, i-1, ' ', 0x0c, 0x0c);
                    }
                  }
                  if (i == 79){
                    for (uint8_t j = 9; j < 14; j++){
                      framebuffer_write(j, i, ' ', 0x0c, 0x0c);
                    }
                    for (uint8_t l = 0; l < 80; l++){
                      for (int m = 0; m < 25; m == 8 ? m += 7: m++){
                        framebuffer_write(m, l, ' ', 0x0, 0x0);
                      }
                    }
                    for (uint8_t j = 0; j < 80; j++){
                      for (uint8_t l = 8; l < 15; l++){
                        framebuffer_write(l, j, ' ', 0x09, 0x09);
                      }

                      for (uint32_t k = 0; k < 70000; k++){
                        io_wait();
                      }
                    }
                    for (uint8_t j = 10; j < 13; j++){
                      framebuffer_write(j, 38, 'S', 0x0, 0x09);
                      framebuffer_write(j, 39, 'U', 0x0, 0x09);
                      framebuffer_write(j, 40, 'C', 0x0, 0x09);
                      framebuffer_write(j, 41, 'C', 0x0, 0x09);
                      framebuffer_write(j, 42, 'E', 0x0, 0x09);
                      framebuffer_write(j, 43, 'S', 0x0, 0x09);
                      framebuffer_write(j, 44, 'S', 0x0, 0x09);
                    }

                    for (uint32_t k = 0; k < 5000000; k++){
                      io_wait();
                    }

                    for (uint8_t l = 0; l < 40; l++){
                      for (uint8_t m = 0; m < 25; m++){
                        framebuffer_write(m, l, ' ', 0x0, 0x0);
                        framebuffer_write(m, 80-l, ' ', 0x0, 0x0);
                      }
                      for (uint32_t k = 0; k < 70000; k++){
                          io_wait();
                      }
                    }
              

                    framebuffer_clear();
                  }
                }
        
              }

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
