#include "lib-header/interrupt.h"
#include "lib-header/framebuffer.h"
#include "lib-header/gdt.h"
#include "lib-header/stdmem.h"
#include "lib-header/idt.h"
#include "lib-header/currentdirectorystack.h"
#include "lib-header/directorystack.h"

uint8_t row_shell = 0;
int current_directory_cluster = ROOT_CLUSTER_NUMBER;
struct CURRENT_DIR_STACK current_dir_stack;
bool initialized_cd_stack = FALSE;

struct TSSEntry _interrupt_tss_entry = {
    .prev_tss = 0,
    .esp0 = 0,
    .ss0 = GDT_KERNEL_DATA_SEGMENT_SELECTOR,
    .unused_register = {0},
};


void io_wait(void) {
    out(0x80, 0);
}

void pic_ack(uint8_t irq) {
    if (irq >= 8)
        out(PIC2_COMMAND, PIC_ACK);
    out(PIC1_COMMAND, PIC_ACK);
}

void pic_remap(void) {
    uint8_t a1, a2;

    // Save masks
    a1 = in(PIC1_DATA); 
    a2 = in(PIC2_DATA);

    // Starts the initialization sequence in cascade mode
    out(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4); 
    io_wait();
    out(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    out(PIC1_DATA, PIC1_OFFSET); // ICW2: Master PIC vector offset
    io_wait();
    out(PIC2_DATA, PIC2_OFFSET); // ICW2: Slave PIC vector offset
    io_wait();
    out(PIC1_DATA, 0b0100);      // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
    io_wait();
    out(PIC2_DATA, 0b0010);      // ICW3: tell Slave PIC its cascade identity (0000 0010)
    io_wait();

    out(PIC1_DATA, ICW4_8086);
    io_wait();
    out(PIC2_DATA, ICW4_8086);
    io_wait();

    // Restore masks
    out(PIC1_DATA, a1);
    out(PIC2_DATA, a2);
}

// void main_interrupt_handler(
//     __attribute__((unused)) struct CPURegister cpu,
//     uint32_t int_number,
//     __attribute__((unused)) struct InterruptStack info
// ) {

//    switch (int_number) {
//         case PAGE_FAULT:
//             __asm__("hlt");
//             break;
//         case PIC1_OFFSET + IRQ_KEYBOARD:
//             keyboard_isr();
//             break;
//     }
// }

void activate_keyboard_interrupt(void) {
    out(PIC1_DATA, PIC_DISABLE_ALL_MASK ^ (1 << IRQ_KEYBOARD));
    out(PIC2_DATA, PIC_DISABLE_ALL_MASK);
}

void set_tss_kernel_current_stack(void) {
    uint32_t stack_ptr;
    // Reading base stack frame instead esp
    __asm__ volatile ("mov %%ebp, %0": "=r"(stack_ptr) : /* <Empty> */);
    // Add 8 because 4 for ret address and other 4 is for stack_ptr variable
    _interrupt_tss_entry.esp0 = stack_ptr + 8; 
}

// struct TSSEntry _interrupt_tss_entry = {
//     .ss0  = GDT_KERNEL_DATA_SEGMENT_SELECTOR,
// };


// TODO: implement puts using framebuffer
void puts(char *str, uint32_t len, uint32_t color) {
    for (uint32_t i = 0; i < len; i++) {
        framebuffer_write(row_shell, i, str[i], color, 0);
    }
}

void puts2(char *str, uint32_t len, uint32_t color) {
    for (uint32_t i = 0; i < len; i++) {
        framebuffer_write(24, i, str[i], color, 0);
    }
}

void puts_long_text(char *str, uint32_t len, uint32_t color) {
    uint8_t j = 0;
    uint8_t k = 0;
    for (uint32_t i = 0; i < len; i++) {
        if (j == 80) {
            row_shell++;
            j = 0;
            k++;
        }
        if (str[i] == '\n') {
            row_shell++;
            j = 0;
            k++;
            continue;
        }
        framebuffer_write(row_shell, j, str[i], color, 0);
        j++;
    }
    addRow(k);
}


void template(void){
    puts("Brother420/", 11, 0x0a);
    framebuffer_write(row_shell, 11, ':', 0x01, 0);
    framebuffer_write(row_shell, 12, '$', 0x0F, 0);
    framebuffer_set_cursor(row_shell, 14);
    row_shell++;
}

/**
 * cd		- Mengganti current working directory (termasuk .. untuk naik)
 * ls		- Menuliskan isi current working directory
 * mkdir	- Membuat sebuah folder kosong baru
 * cat		- Menuliskan sebuah file sebagai text file ke layar (Gunakan format LF newline)
 * cp		- Mengcopy suatu file
 * rm		- Menghapus suatu file atau folder kosong
 * mv		- Memindah dan merename lokasi file/folder
 * whereis	- Mencari file/folder dengan nama yang sama diseluruh file system

 * @return 0 cd (spasi)
 * @return 1 ls
 * @return 2 mkdir (spasi)
 * @return 3 cat (spasi)
 * @return 4 cp (spasi)
 * @return 5 rm (spasi)
 * @return 6 mv (spasi)
 * @return 7 whereis (spasi)
*/
uint8_t getCommandInput(char *input, uint8_t len){
    char *command[] = {"cd ", "ls", "mkdir ", "cat ", "cp ", "rm ", "mv ", "whereis "};
    uint8_t command_len[] = {3, 2, 6, 4, 3, 3, 3, 8};
    for (uint8_t i = 0; i < 8; i++){
        if (len >= command_len[i] && memcmp(input, command[i], command_len[i]) == 0){
            return i;
        }
        framebuffer_write(23, i, input[i], 0x0f, 0);
    }
    return 255;
}

void puts_line(char *input, int row, int col, int length, int color){
    for (int i = 0; i < length; i++){
        framebuffer_write(row, col + i, input[i], color, 0);
    }
}


void command_call_ls(void){
    struct FAT32DriverState state_driver;
    read_clusters(&state_driver.dir_table_buf, current_directory_cluster, 1);
    read_clusters(&state_driver.fat_table, 1, 1);
    
    int col = 0;
    int row_add_needed = 1;

    for (int i = 1; i < 64; i++){

        if (state_driver.dir_table_buf.table[i].user_attribute == UATTR_NOT_EMPTY){
            if (col == 5){
                col = 0;
                row_shell++;
                row_add_needed++;
            }
            puts_line(state_driver.dir_table_buf.table[i].name, row_shell, col*16, 8, 0x0f);
            col++;
        }
    }
    row_shell++;

    addRow(row_add_needed);

}

void command_call_mkdir(char *dirCommandName){
    
    struct FAT32DriverRequest request = {
        .buf                    = 0,
        .ext                    = "\0\0\0",
        .parent_cluster_number  = current_directory_cluster,
        .buffer_size            = 0,
    };
    for (int i = 6; i < 14; i++){
        request.name[i-6] = dirCommandName[i];
    }
    write(request);
}

void command_call_cp(char *cpCommandName){
    struct FAT32DriverState state_driver;
    read_clusters(&state_driver.dir_table_buf, current_directory_cluster, 1);
    read_clusters(&state_driver.fat_table, 1, 1);

    struct FAT32DriverRequest request = {
        .parent_cluster_number  = current_directory_cluster,
    };

    uint8_t i = 0;
    uint8_t j = 0;
    while ((cpCommandName[i+3] != '.') && (cpCommandName[i+3] != ' ') && (i < 8)){
        request.name[i] = cpCommandName[i+3];
        i++;
    }

    if (cpCommandName[i+3] == ' '){
        // Error karena tidak ada extensi
        return;
    }

    if (i == 8 && cpCommandName[i+3] != '.'){
        // Error karena nama file terlalu panjang
        return;
    }

    if (i < 8){
        for (int z = i; z < 8; z++){
            request.name[z] = '\0';
        }
    }

    j = i+1;
    while ((cpCommandName[j+3] != ' ') && (j-i-1 < 3)){
        request.ext[j-i-1] = cpCommandName[j+3];
        j++;
    }

    if (j-i-1 == 3 && cpCommandName[j+3] != ' '){
        // Error karena extensi terlalu panjang
        return;
    }

    if (j-i-1 < 3){
        for (int z = j-i-1; z < 3; z++){
            request.ext[z] = '\0';
        }
    }


    for (int m = 1; m < 64; m++){
        if (state_driver.dir_table_buf.table[m].user_attribute == UATTR_NOT_EMPTY){
            if (memcmp(state_driver.dir_table_buf.table[m].name, request.name, 8) == 0 && 
                memcmp(state_driver.dir_table_buf.table[m].ext, request.ext, 3) == 0){
                // File ditemukan
                request.buffer_size = state_driver.dir_table_buf.table[m].filesize;
                read(request);
                break;      
            }
        }
    }

    uint8_t k = 0;
    uint8_t l = 0;
    while ((cpCommandName[j+4] != '.') && (cpCommandName[j+4] != ' ') && (k < 8)){
        request.name[k] = cpCommandName[j+4];
        k++;
        j++;
    }

    if (cpCommandName[j+4] == ' '){
        // Error karena tidak ada extensi
        return;
    }

    if (k == 8 && cpCommandName[j+4] != '.'){
        // Error karena nama file terlalu panjang
        return;
    }

    if (k < 8){
        for (int z = k; z < 8; z++){
            request.name[z] = '\0';
        }
    }

    while ((cpCommandName[l+j+5] != ' ') && cpCommandName[l+j+5] != '\0' && (l < 3)){
        request.ext[l] = cpCommandName[l+j+5];
        l++;
    }

    if (l == 3 && ((cpCommandName[l+j+5] != ' ') && (cpCommandName[l+j+5] != '\0'))){
        // Error karena extensi terlalu panjang
        return;
    }

    if (l < 3){
        for (int z = l; z < 3; z++){
            request.ext[z] = '\0';
        }
    }

    write(request);


}


void command_call_rm(char *rmCommandName){
    struct FAT32DriverState state_driver;
    read_clusters(&state_driver.dir_table_buf, current_directory_cluster, 1);
    read_clusters(&state_driver.fat_table, 1, 1);
    bool isFolder = FALSE;

    struct FAT32DriverRequest request = {
        .parent_cluster_number  = current_directory_cluster,
    };

    uint8_t i = 0;
    uint8_t j = 0;
    while ((rmCommandName[i+3] != '.') && (rmCommandName[i+3] != ' ') && (i < 8)){
        request.name[i] = rmCommandName[i+3];
        i++;
    }

    if (rmCommandName[i+3] == ' '){
        // Error karena tidak ada extensi
        return;
    }

    if (rmCommandName[i+3] == '\0'){
        // berarti folder
        isFolder = TRUE;
        for (int z = 0; z < 3; z++){
            request.ext[z] = '\0';
        }
    }

    if (i < 8){
        for (int z = i; z < 8; z++){
            request.name[z] = '\0';
        }
    }

    if (!isFolder){
        j = i+1;
        while ((rmCommandName[j+3] != ' ') && (j-i-1 < 3)){
            request.ext[j-i-1] = rmCommandName[j+3];
            j++;
        }

        if (j-i-1 == 3 && rmCommandName[j+3] != '\0' && rmCommandName[j+3] != ' '){
            // Error karena extensi terlalu panjang
            return;
        }

        if (j-i-1 < 3){
            for (int z = j-i-1; z < 3; z++){
                request.ext[z] = '\0';
            }
        }
    }

    for (int m = 1; m < 64; m++){
        if (state_driver.dir_table_buf.table[m].user_attribute == UATTR_NOT_EMPTY){
            if (memcmp(state_driver.dir_table_buf.table[m].name, request.name, 8) == 0 && 
                memcmp(state_driver.dir_table_buf.table[m].ext, request.ext, 3) == 0){
                // File ditemukan
                request.buffer_size = state_driver.dir_table_buf.table[m].filesize;
                delete(request);
                break;      
            }
        }
    }
}


void command_call_cat(char *rmCommandName){
    // asumsi file only

    struct FAT32DriverState state_driver;
    read_clusters(&state_driver.dir_table_buf, current_directory_cluster, 1);
    read_clusters(&state_driver.fat_table, 1, 1);

    struct FAT32DriverRequest request = {
        .parent_cluster_number  = current_directory_cluster,
    };

    uint8_t i = 0;
    uint8_t j = 0;
    while ((rmCommandName[i+4] != '.') && (rmCommandName[i+4] != ' ') && (i < 8)){
        request.name[i] = rmCommandName[i+4];
        i++;
    }

    if (rmCommandName[i+4] == ' '){
        // Error karena tidak ada extensi
        return;
    }

    if (i < 8){
        for (int z = i; z < 8; z++){
            request.name[z] = '\0';
        }
    }

    j = i+1;
    while ((rmCommandName[j+4] != ' ') && (j-i-1 < 3)){
        request.ext[j-i-1] = rmCommandName[j+4];
        j++;
    }

    if (j-i-1 == 4 && rmCommandName[j+4] != '\0' && rmCommandName[j+4] != ' '){
        // Error karena extensi terlalu panjang
        return;
    }

    if (j-i-1 < 3){
        for (int z = j-i-1; z < 3; z++){
            request.ext[z] = '\0';
        }
    }

    for (int m = 1; m < 64; m++){
        if (state_driver.dir_table_buf.table[m].user_attribute == UATTR_NOT_EMPTY){
            if (memcmp(state_driver.dir_table_buf.table[m].name, request.name, 8) == 0 && 
                memcmp(state_driver.dir_table_buf.table[m].ext, request.ext, 3) == 0){
                // File ditemukan
                request.buffer_size = state_driver.dir_table_buf.table[m].filesize;
                read(request);
                break;      
            }
        }
    }

    puts_long_text(request.buf, request.buffer_size, 0x0e);

    framebuffer_write(23, 79, 'L', 0x0F, 0x00);

}

/**
 * Command cd <path> dilakukan untuk pindah ke direktori path
 * @param path direktori tujuan yang hanya satu kata
 * @return 0 jika berhasil, 1 jika gagal
*/
uint8_t command_call_cd(char *path){
    if (!initialized_cd_stack){
        initialized_cd_stack = TRUE;
        init_current_dir_stack(&current_dir_stack);
    }

    // for one time only
    struct FAT32DriverState state_driver;
    read_clusters(&state_driver.dir_table_buf, current_directory_cluster, 1);
    read_clusters(&state_driver.fat_table, 1, 1);

    char path_name[8];
    char path_ext[3];
    memcpy(path_name, path, 8);
    memcpy(path_ext, "\0\0\0", 3);

    if (memcmp(path, "..", 2) == 0){
        // Pindah ke parent directory
        if (current_directory_cluster == 2){
            return 1;
        }
        pop_current_dir(&current_dir_stack);
        current_directory_cluster = get_top_current_dir(&current_dir_stack);
        return 0;
    }

    for (int i = 0; i < 64; i++){
        if (state_driver.dir_table_buf.table[i].user_attribute == UATTR_NOT_EMPTY){
            if (memcmp(state_driver.dir_table_buf.table[i].name, path_name, 8) == 0 && 
                memcmp(state_driver.dir_table_buf.table[i].ext, path_ext, 3) == 0){
                // Folder ditemukan
                current_directory_cluster = state_driver.dir_table_buf.table[i].cluster_high << 16 | state_driver.dir_table_buf.table[i].cluster_low;
                push_current_dir(&current_dir_stack, current_directory_cluster);
                return 0;
            }
        }
    } 
    return 1;
}


// char* get_file_name(char *path){
//     char file_name[8];
//     uint8_t i = 0;
//     while ((path[i] != '.') && (path[i+3] != ' ') && (i < 8)){
//         file_name[i] = path[i+3];
//         i++;
//     }
//     return file_name;
// }


/**
 * Command mv file/folder <path> dilakukan untuk merename file/folder dan memindahkannya ke path
 * @param path nama file/folder dan direktori tujuan yang MASIH satu kata
*/
void command_call_mv(char *path){
    struct FAT32DriverState state_driver;
    read_clusters(&state_driver.dir_table_buf, current_directory_cluster, 1);
    read_clusters(&state_driver.fat_table, 1, 1);
    bool isFolder = FALSE;

    struct FAT32DriverRequest request = {
        .parent_cluster_number  = current_directory_cluster,
    };

    char new_name[8];
    char new_ext[3];
    bool fileFound = FALSE;
    // bool validName = FALSE;

    uint8_t i = 0;
    uint8_t j = 0;
    while ((path[i] != '.') && (path[i] != ' ') && (i < 8)){
        request.name[i] = path[i];
        i++;
    }

    if (i == 8 && path[i] != '.' && path[i] != ' '){
        // berarti kepanjangan
        return;
    }

    if (path[i] == ' '){
        // berarti folder
        isFolder = TRUE;
        for (int z = 0; z < 3; z++){
            request.ext[z] = '\0';
        }
    }

    if (i < 8){
        for (int z = i; z < 8; z++){
            request.name[z] = '\0';
        }
    }

    j = i+1;

    if (!isFolder){
        
        while ((path[j] != ' ') && (j-i-1 < 3)){
            request.ext[j-i-1] = path[j];
            j++;
        }

        if (j-i-1 == 3 && path[j] != '\0' && path[j] != ' '){
            // Error karena extensi terlalu panjang
            return;
        }

        if (j-i-1 < 3){
            for (int z = j-i-1; z < 3; z++){
                request.ext[z] = '\0';
            }
        }
    }

    // saat ini dah didapat mv file/folder 
    int m;
    for (m = 1; m < 64; m++){
        if (state_driver.dir_table_buf.table[m].user_attribute == UATTR_NOT_EMPTY){
            if (memcmp(state_driver.dir_table_buf.table[m].name, request.name, 8) == 0 && 
                memcmp(state_driver.dir_table_buf.table[m].ext, request.ext, 3) == 0){
                // File ditemukan
                request.buffer_size = state_driver.dir_table_buf.table[m].filesize;
                fileFound = TRUE;
                break;      
            }
        }
    }

    if (j + 1 == '/'){
        // TODO pindah direktori
    } else {

        uint8_t k = isFolder ? j : j+1;
        uint8_t l = 0;

        if (path[k] == '\0' || path[k] == ' '){
            // Error karena nama file tidak boleh kosong
            return;
        }

        while ((path[k] != ' ') && (k-j-1 < 8) && (path[k] != '\0') && (path[k] != '.')){
            new_name[l] = path[k];
            k++;
            l++;
        }

        if (l == 8 && path[k] != '\0' && path[k] != ' ' && path[k] != '.'){
            // Error karena nama file terlalu panjang
            return;
        }

        for (uint8_t z = l; z < 8; z++){
            new_name[z] = '\0';
        }

        if (isFolder){
            for (int z = 0; z < 3; z++){
                new_ext[z] = '\0';
            }
        } else {
            j = k+1;
            while ((path[j] != ' ') && (j-k-1 < 3)){
                new_ext[j-k-1] = path[j];
                j++;
            }

            if (j-k-1 == 3 && path[j] != '\0' && path[j] != ' '){
                // Error karena extensi terlalu panjang
                return;
            }

            if (j-k-1 < 3){
                for (int z = j-k-1; z < 3; z++){
                    new_ext[z] = '\0';
                }
            }
        }

        if (fileFound){
            memcpy(state_driver.dir_table_buf.table[m].name, new_name, 8);
            memcpy(state_driver.dir_table_buf.table[m].ext, new_ext, 3);
            write_clusters(&state_driver.dir_table_buf, current_directory_cluster, 1);
            
        }

    }

    
}


void command_call_multi_cd(char *path){
    struct FAT32DriverState state_driver;
    read_clusters(&state_driver.dir_table_buf, current_directory_cluster, 1);
    read_clusters(&state_driver.fat_table, 1, 1);

    uint8_t i = 0;
    uint8_t j = 0;
    while (path[i] != '\0'){
        if (path[i] == '/'){
            j = i;
        }
        i++;
    }

    if (j == 0){
        // berarti hanya satu direktori
        command_call_cd(path);
    } else {
        char new_path[64];
        memcpy(new_path+3, path, j);
        new_path[j+3] = '\0';
        command_call_cd(new_path);
        command_call_multi_cd(path+j+1);
    }
}

void syscall(struct CPURegister cpu, __attribute__((unused)) struct InterruptStack info) {
    if (cpu.eax == 0) {
        struct FAT32DriverRequest request = *(struct FAT32DriverRequest*) cpu.ebx;
        *((int8_t*) cpu.ecx) = read(request);
        // if (*((int8_t*) cpu.ecx) == 0){
        //     puts(request.buf, request.buffer_size, 0x0e);
        // }
    } else if (cpu.eax == 4) {
        template();
        clear_keyboard_buffer();
        keyboard_state_activate();
        __asm__("sti"); // Due IRQ is disabled when main_interrupt_handler() called
        while (is_keyboard_blocking());
        char buf[KEYBOARD_BUFFER_SIZE];
        get_keyboard_buffer(buf);
        memcpy((char *) cpu.ebx, buf, cpu.ecx);
    } else if (cpu.eax == 5) {
        // ini berarti di-enter dari syscall 4
        
        uint8_t command = getCommandInput((char *) cpu.ebx, cpu.ecx);
        
        switch (command){
            case 0:
                // cd
                framebuffer_write(0, 79, '0', 0x0f, 0);
                command_call_multi_cd(((char *) cpu.ebx) + 3);
                break;
            case 1:
                // ls
                framebuffer_write(0, 79, '1', 0x0f, 0);
                command_call_ls();
                break;
            case 2:
                // mkdir
                framebuffer_write(0, 79, '2', 0x0f, 0);
                command_call_mkdir((char *) cpu.ebx);
                break;
            case 3:
                // cat
                framebuffer_write(0, 79, '3', 0x0f, 0);
                command_call_cat((char *) cpu.ebx);
                break;
            case 4:
                // cp
                framebuffer_write(0, 79, '4', 0x0f, 0);
                command_call_cp((char *) cpu.ebx);
                break;
            case 5:
                // rm
                framebuffer_write(0, 79, '5', 0x0f, 0);
                command_call_rm((char *) cpu.ebx);
                break;
            case 6:
                // mv
                framebuffer_write(0, 79, '6', 0x0f, 0);
                command_call_mv(((char *) cpu.ebx) + 3);
                break;
            case 7:
                // whereis
                framebuffer_write(0, 79, '7', 0x0f, 0);
                break;
            default:
                // command not found
                framebuffer_write(0, 79, 'x', 0x0f, 0);
                break;
        }
            

        puts2((char *) cpu.ebx, cpu.ecx, cpu.edx); // Modified puts() on kernel side
    }
}


void main_interrupt_handler(struct CPURegister cpu, uint32_t int_number, struct InterruptStack info) {    
    
    switch (int_number) {

        case PIC1_OFFSET + IRQ_KEYBOARD:
            keyboard_isr();
            break;
        case 0x30:
            syscall(cpu, info);
            break;

    };
}

