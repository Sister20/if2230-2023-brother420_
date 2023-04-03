// #include "lib-header/keyboard.h"
#include "lib-header/portio.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"
#include "lib-header/gdt.h"
#include "lib-header/framebuffer.h"
#include "lib-header/kernel_loader.h"
#include "lib-header/idt.h"
#include "lib-header/keyboard.h"
#include "lib-header/interrupt.h"
#include "lib-header/fat32.h"
#include "lib-header/paging.h"


void write_splash_screen3();

void kernel_setup(void) {
    enter_protected_mode(&_gdt_gdtr);
    pic_remap();
    initialize_idt();
    activate_keyboard_interrupt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);
    initialize_filesystem_fat32();
    gdt_install_tss();
    set_tss_register();

    // Allocate first 4 MiB virtual memory
    allocate_single_user_page_frame((uint8_t*) 0);

    // Write shell into memory (assuming shell is less than 1 MiB)
    struct FAT32DriverRequest request = {
        .buf                   = (uint8_t*) 0,
        .name                  = "shell",
        .ext                   = "\0\0\0",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size           = 0x100000,
    };
    read(request);

    // Set TSS $esp pointer and jump into shell 
    set_tss_kernel_current_stack();
    kernel_execute_user_program((uint8_t *) 0); // kayaknya di sini salah
    int i = 0;
    i++;
    while (TRUE);
}


/* Struktur folder *
 * ROOT
 *  ikanaide
 *      kanol
 *          daijoubu
 *          perusak
 *  BRO
 *      brother
 *      b420
 *  destroys
 */
// void kernel_setup(void) {

//     /* Enter protected mode */
//     enter_protected_mode(&_gdt_gdtr);


//     /* Remap PIC */
//     pic_remap();
//     initialize_idt();
//     framebuffer_clear();
//     framebuffer_set_cursor(0, 0);
//     initialize_filesystem_fat32();


//     /* Initialize keyboard */
//     activate_keyboard_interrupt();


//     /* Initialize cbuf for debugging */
//     struct ClusterBuffer cbuf[5];
//     for (uint32_t i = 0; i < 5; i++)
//         for (uint32_t j = 0; j < CLUSTER_SIZE; j++)
//             cbuf[i].buf[j] = i + 'a';


//     /* Initialize cbufs for debugging */
//     struct ClusterBuffer cbufs[6];
//     for (uint32_t i = 0; i < 5; i++)
//         for (uint32_t j = 0; j < CLUSTER_SIZE; j++)
//             cbufs[i].buf[j] = i + '1';
//     cbufs[5].buf[0] = 'E';
//     cbufs[5].buf[1] = 'N';
//     cbufs[5].buf[2] = 'D';


//     /* Initialize request for debugging */
//     struct FAT32DriverRequest request = {
//         .buf                   = cbuf,
//         .name                  = "ikanaide",
//         .ext                   = "uwu",
//         .parent_cluster_number = ROOT_CLUSTER_NUMBER,
//         .buffer_size           = 0,
//     } ;


//     /* Folder ikanaide attached to ROOT */
//     write(request);  // Create folder "ikanaide"


//     /* Folder BRO attached to ROOT */
//     memcpy(request.name, "BRO\0\0\0\0\0", 8);
//     write(request);  // Create folder "BRO"


//     /* Folder brother */
//     request.parent_cluster_number = 4;
//     memcpy(request.name, "brother\0", 8);
//     write(request);  // Create folder "brother"


//     /* Folder b420 */
//     request.parent_cluster_number = 4;
//     memcpy(request.name, "b420---\0", 8);
//     write(request);  // Create folder "b420"


//     /* Folder destroys */
//     request.parent_cluster_number = 2;
//     memcpy(request.name, "destroys", 8);
//     write(request);  // Create folder "destroys"


//     /* Folder kano1 attached to folder ikanaide */
//     request.parent_cluster_number = 3;
//     memcpy(request.name, "kano1\0\0\0", 8);
//     write(request);  // Create folder "kano1"


//     /* Delete debug for folder destroys (should be success) */
//     // memcpy(request.name, "destroys", 8);
//     // request.buffer_size = 0;
//     // request.parent_cluster_number = 2;
//     // delete(request);


//     /* File daijoubu attached to folder kanol */
//     memcpy(request.name, "daijoubu", 8);
//     request.buffer_size = 5*CLUSTER_SIZE;
//     request.parent_cluster_number = 8;
//     write(request);  // Create fragmented file "daijoubu"


//     /* File perusak attached to folder kanol */
//     request.buf = cbufs;
//     memcpy(request.name, "perusak-", 8);
//     request.buffer_size = 5 * CLUSTER_SIZE + 3;
//     write(request);  // Create fragmented file "perusak"


//     /* Delete debug for file perusak (should be success) */
//     // memcpy(request.name, "perusak-", 8);
//     // memcpy(request.ext, "uwu", 3);
//     // request.buffer_size = 6 * CLUSTER_SIZE;
//     // request.parent_cluster_number = 8;
//     // delete(request);


//     /* Delete debug for folder kanol (should be fail) */
//     // memcpy(request.name, "kanol", 8);
//     // request.buffer_size = 0;
//     // request.parent_cluster_number = 3;
//     // delete(request);


//     /* File duplicate Folder BRO attached to ROOT */
//     request.buffer_size = 0;
//     request.parent_cluster_number = ROOT_CLUSTER_NUMBER;
//     memcpy(request.name, "BRO\0\0\0\0\0", 8);
//     memcpy(request.ext, "uwO", 3);
//     uint8_t test = write(request);  // Create folder "BRO"
//     test += 3; // Buat unused variable


//     /* Reset request.buf */
//     memcpy(request.ext, "uwu", 3);
//     for (uint32_t i = 0; i < 5; i++)
//         for (uint32_t j = 0; j < CLUSTER_SIZE; j++)
//             cbuf[i].buf[j] = '0';
//     cbufs[5].buf[0] = 'E';
//     cbufs[5].buf[1] = 'N';
//     cbufs[5].buf[2] = 'D';
//     request.buf = cbufs;


//     /* Read file perusak */
//     struct ClusterBuffer readcbuf;
//     read_clusters(&readcbuf, ROOT_CLUSTER_NUMBER + 10, 1); 
//     // If read properly, readcbuf should filled with 'a'


//     /* Test read FAIL due not enough buffer size */
//     request.parent_cluster_number = 4;
//     request.buffer_size = CLUSTER_SIZE;
//     int8_t debug01 = read(request);   // Failed read due not enough buffer size


//     /* Test read FAIL due not valid parent_cluster */
//     request.parent_cluster_number = 20;
//     request.buffer_size = CLUSTER_SIZE;
//     int8_t debug03 = read(request);   // Failed read due not valid parent_cluster
    

//     /* Test read SUCCESS */
//     request.buffer_size = 5*CLUSTER_SIZE;
//     int8_t debug02 = read(request);   // Success read on file "daijoubu"
    

//     /* Buat unused variable saja */
//     debug01 += debug02 + debug03;
//     write_splash_screen3();
//     while (TRUE && !altF4()) {
//         keyboard_state_activate(); 
//         if (restoreSplashScreen()) {
//             write_splash_screen3();

//         }
//     }
    
// }


// void kernel_setup(void) {
//     enter_protected_mode(&_gdt_gdtr);
//     pic_remap();
//     initialize_idt();
//     framebuffer_clear();
//     framebuffer_set_cursor(0, 0);
//     while (TRUE) 
//       keyboard_state_activate();
// }


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
