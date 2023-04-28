#include "../lib-header/stdtype.h"
#include "../lib-header/fat32.h"
#include "../lib-header/currentdirectorystack.h"

uint8_t row_shell = 0;
int current_directory_cluster = ROOT_CLUSTER_NUMBER;
struct CURRENT_DIR_STACK current_dir_stack;
bool initialized_cd_stack = FALSE;

void* memcpy(void* restrict dest, const void* restrict src, size_t n) {
    uint8_t *dstbuf       = (uint8_t*) dest;
    const uint8_t *srcbuf = (const uint8_t*) src;
    for (size_t i = 0; i < n; i++)
        dstbuf[i] = srcbuf[i];
    return dstbuf;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *buf1 = (const uint8_t*) s1;
    const uint8_t *buf2 = (const uint8_t*) s2;
    for (size_t i = 0; i < n; i++) {
        if (buf1[i] < buf2[i])
            return -1;
        else if (buf1[i] > buf2[i])
            return 1;
    }

    return 0;
}

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    __asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
    // Note : gcc usually use %eax as intermediate register,
    //        so it need to be the last one to mov
    __asm__ volatile("int $0x30");
}

// TODO: pindahin semua fungsi dari interupt.c

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
    }
    return 255;
}

void command_call_ls(void){
    struct FAT32DriverState state_driver;
    // read_clusters(&state_driver.dir_table_buf, current_directory_cluster, 1);
    syscall(8, (uint32_t) &state_driver.dir_table_buf, current_directory_cluster, 1);
    
    int col = 0;
    int row_add_needed = 1;

    for (int i = 1; i < 64; i++){

        if (state_driver.dir_table_buf.table[i].user_attribute == UATTR_NOT_EMPTY){
            if (col == 5){
                col = 0;
                row_shell++;
                row_add_needed++;
            }
            syscall(6, (uint32_t) state_driver.dir_table_buf.table[i].name, row_shell, col*16);
            col++;
        }
    }
    row_shell++;

    syscall(7, row_add_needed, 0, 0);

}

/**
 * Command cd <path> dilakukan untuk pindah ke direktori path
 * @param path direktori tujuan yang hanya satu kata
 * @return 0 jika berhasil, 1 jika gagal
*/
uint8_t command_call_cd(char *path){
    if (!initialized_cd_stack){
        initialized_cd_stack = TRUE;
        // init_current_dir_stack(&current_dir_stack);
        syscall(9, (uint32_t) &current_dir_stack, 0, 0);
    }

    // for one time only
    struct FAT32DriverState state_driver;
    // read_clusters(&state_driver.dir_table_buf, current_directory_cluster, 1);
    syscall(8, (uint32_t) &state_driver.dir_table_buf, current_directory_cluster, 1);

    char path_name[8];
    char path_ext[3];
    memcpy(path_name, path, 8);
    memcpy(path_ext, "\0\0\0", 3);

    if (memcmp(path, "..", 2) == 0){
        // Pindah ke parent directory
        if (current_directory_cluster == 2){
            return 1;
        }
        // pop_current_dir(&current_dir_stack);
        // current_directory_cluster = get_top_current_dir(&current_dir_stack);
        syscall(10, (uint32_t) &current_dir_stack, (uint32_t) &current_directory_cluster, 0);
        return 0;
    }

    for (int i = 0; i < 64; i++){
        if (state_driver.dir_table_buf.table[i].user_attribute == UATTR_NOT_EMPTY){
            if (memcmp(state_driver.dir_table_buf.table[i].name, path_name, 8) == 0 && 
                memcmp(state_driver.dir_table_buf.table[i].ext, path_ext, 3) == 0){
                // Folder ditemukan
                current_directory_cluster = state_driver.dir_table_buf.table[i].cluster_high << 16 | state_driver.dir_table_buf.table[i].cluster_low;
                // push_current_dir(&current_dir_stack, current_directory_cluster);
                syscall(11, (uint32_t) &current_dir_stack, (uint32_t) current_directory_cluster, 0);
                return 0;
            }
        }
    } 
    return 1;
}

void command_call_multi_cd(char *path){

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
        memcpy(new_path, path, j);
        new_path[j] = '\0';
        command_call_cd(new_path);
        command_call_multi_cd(path+j+1);
    }
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
    // write(request);
    syscall(2, (uint32_t) &request, 0, 0);
}


void command_call_cat(char *rmCommandName){
    // asumsi file only

    int32_t retCode;
    struct FAT32DriverState state_driver;
    // read_clusters(&state_driver.dir_table_buf, current_directory_cluster, 1);
    syscall(8, (uint32_t) &state_driver.dir_table_buf, current_directory_cluster, 1);
    
    struct ClusterBuffer cbuf[4];

    struct FAT32DriverRequest request = {
        .parent_cluster_number  = current_directory_cluster,
        .buf                    = cbuf,
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
                // read(request);
                syscall(0, (uint32_t) &request, (uint32_t) &retCode, 0);
                break;      
            }
        }
    }

    // puts_long_text(request.buf, request.buffer_size, 0x0e);
    syscall(12, (uint32_t) request.buf, request.buffer_size, (uint32_t) &row_shell);
}


int main(void) {
    struct ClusterBuffer cl           = {0};
    struct FAT32DriverRequest request = {
        .buf                   = &cl,
        .name                  = "ikanaide",
        .ext                   = "\0\0\0",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size           = 20,
    };
    int32_t retcode;
    syscall(0, (uint32_t) &request, (uint32_t) &retcode, 0);
    if (retcode == 0)
        syscall(5, (uint32_t) "eswoss\n", 4, 0xF);

    char buf[16];

    uint8_t command;
    while (TRUE) {
        syscall(4, (uint32_t) buf, 0x20, (uint32_t) row_shell);
        row_shell++;
        command = getCommandInput((char*) buf, 16);
        
        // TODO: prosess
        switch (command){
            case 0:
                // cd
                command_call_multi_cd(((char *) buf) + 3);
                break;
            case 1:
                // ls
                command_call_ls();
                break;
            case 2:
                // mkdir
                command_call_mkdir((char *) buf);
                break;
            case 3:
                // cat
                command_call_cat((char *) buf);
                break;
            case 4:
                // cp
                // framebuffer_write(0, 79, '4', 0x0f, 0);
                // command_call_cp((char *) buf);
                break;
            case 5:
                // rm
                // framebuffer_write(0, 79, '5', 0x0f, 0);
                // command_call_rm((char *) buf);
                break;
            case 6:
                // mv
                // framebuffer_write(0, 79, '6', 0x0f, 0);
                // command_call_mv(((char *) buf) + 3);
                break;
            case 7:
                // whereis
                // framebuffer_write(0, 79, '7', 0x0f, 0);
                break;
            default:
                // command not found
                // framebuffer_write(0, 79, 'x', 0x0f, 0);
                break;
        }

        syscall(5, (uint32_t) buf, 0x20, 0xF);
    }

    return 0;
}
