#include "../lib-header/stdtype.h"
#include "../lib-header/fat32.h"
#include "../lib-header/currentdirectorystack.h"
#include "../lib-header/directorystack.h"

uint8_t row_shell = 0;
int current_directory_cluster = ROOT_CLUSTER_NUMBER;
struct CURRENT_DIR_STACK current_dir_stack;
struct DIR_STACK dirRet;
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

char* dirStackConverter(struct CURRENT_DIR_STACK *dir){
    struct FAT32DriverState state_driver;
    char temp[255];
    char * cwd = temp;
    int i = 1;
    int j = 0;
    int k = 0;
    while(i <= dir->top){
        cwd[j] = '/';
        j++;
        syscall(8, (uint32_t) &state_driver.dir_table_buf, dir->cd_stack[i], 1);
        memcpy(dir->cd_stack_name[i], state_driver.dir_table_buf.table[0].name, 8);
        while(dir->cd_stack_name[i][k] != '\0'){
            cwd[j] = dir->cd_stack_name[i][k];
            j++;
            k++;
        }
        k = 0;
        i++;
    }
    cwd[j] = '\0';
    return cwd;
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
 * @return 1 ls -a
 * @return 2 mkdir (spasi)
 * @return 3 cat (spasi)
 * @return 4 cp (spasi)
 * @return 5 rm (spasi)
 * @return 6 mv (spasi)
 * @return 7 whereis (spasi)
 * @return 8 ls -ff
 * @return 9 ls -F
 * @return 10 ls -f
 * @return 11 ls
 * @return 255 command not found
*/
uint8_t getCommandInput(char *input, uint8_t len){
    char *command[] = {"cd ", "ls -a", "mkdir ", "cat ", "cp ", "rm ", "mv ", "whereis ",
                        "ls -ff", "ls -F", "ls -f", "ls", "cls"};
    uint8_t command_len[] = {3, 5, 6, 4, 3, 3, 3, 8, 6, 5, 5, 2, 3};
    for (uint8_t i = 0; i < 13; i++){
        if (len >= command_len[i] && memcmp(input, command[i], command_len[i]) == 0){
            return i;
        }
    }
    return 255;
}

/**
 * @param type 0 ls
 * @param type 1 ls -F
 * @param type 2 ls -f
 * @param type 3 ls -ff
 * @param type 4 ls -a
*/
void command_call_ls(uint8_t type){
    struct FAT32DriverState state_driver;
    // read_clusters(&state_driver.dir_table_buf, current_directory_cluster, 1);
    syscall(8, (uint32_t) &state_driver.dir_table_buf, current_directory_cluster, 1);
    
    int col = 0;
    int row_add_needed = 1;
    bool isFile = FALSE;
    
    for (int i = 1; i < 64; i++){
        char nprinter[12] = {0};
        char aprinter[12] = {0};
        char Fprinter[12] = {0};
        char fprinter[12] = {0};
        char ffprinter[12] = {0};

        if (state_driver.dir_table_buf.table[i].user_attribute == UATTR_NOT_EMPTY){
            if (col == 4 && type >= 3){
                col = 0;
                row_shell++;
                row_add_needed++;
            } 
            else if (col == 5){
                col = 0;
                row_shell++;
                row_add_needed++;
            }
            memcpy(aprinter, state_driver.dir_table_buf.table[i].name, 8);
            memcpy(nprinter, state_driver.dir_table_buf.table[i].name, 8);
            memcpy(aprinter+9, state_driver.dir_table_buf.table[i].ext, 3);
            if (memcmp(state_driver.dir_table_buf.table[i].ext, "\0\0\0", 3) != 0){
                // File
                isFile = TRUE;
                aprinter[8] = '.';
                memcpy(fprinter, aprinter, 8);
                memcpy(ffprinter, aprinter, 12);
            } else {
                // Folder
                isFile = FALSE;
                memcpy(Fprinter, aprinter, 8);
            }

            switch (type){
            case 0:
                syscall(6, (uint32_t) nprinter, row_shell, col*16);
                break;
            case 1:
                syscall(6, (uint32_t) Fprinter, row_shell, col*16);
                break;
            case 2:
                syscall(6, (uint32_t) fprinter, row_shell, col*16);
                break;
            case 3:
                syscall(6, (uint32_t) ffprinter, row_shell, col*20);
                break;
            case 4:
                syscall(6, (uint32_t) aprinter, row_shell, col*20);
                break;
            default:
                break;
            }
            
            if (isFile && type != 1){
                col++;
            } else if (!isFile && type == 1){
                col++;
            } else if (type == 0 || type == 4){
                col++;
            }
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

    for (int i = 1; i < 64; i++){
        if (state_driver.dir_table_buf.table[i].user_attribute == UATTR_NOT_EMPTY){
            if (memcmp(state_driver.dir_table_buf.table[i].name, path_name, 8) == 0 && 
                memcmp(state_driver.dir_table_buf.table[i].ext, path_ext, 3) == 0){
                // Folder ditemukan
                current_directory_cluster = state_driver.dir_table_buf.table[i].cluster_high << 16 | state_driver.dir_table_buf.table[i].cluster_low;
                // push_current_dir(&current_dir_stack, current_directory_cluster);
                syscall(11, (uint32_t) &current_dir_stack, (uint32_t) current_directory_cluster, (uint32_t) path_name);
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
            break;
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
        uint8_t retVal = command_call_cd(new_path);
        if (retVal == 0)
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

void command_call_cp(char *cpCommandName){
    uint32_t retCode;
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
                // read(request);
                syscall(0, (uint32_t) &request, (uint32_t) &retCode, 0);
                
                break;      
            }
        }
        if (m == 63){
            // File tidak ditemukan
            return;
        }
    }

    uint8_t k = 0;
    uint8_t l = 0;
    while ((cpCommandName[j+4] != '.') && (cpCommandName[j+4] != ' ') && (cpCommandName[j+4] != '\0') && (k < 8)){
        request.name[k] = cpCommandName[j+4];
        k++;
        j++;
    }

    if (cpCommandName[j+4] == ' '){
        // Error karena tidak ada extensi
        return;
    }

    if (k == 8 && cpCommandName[j+4] != '.' && cpCommandName[j+4] != '\0'){
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

    if (l == 0){
        // do nothing karena ext tidak berubah
    } else if (l < 3){
        for (int z = l; z < 3; z++){
            request.ext[z] = '\0';
        }
    }

    // write(request);
    syscall(2, (uint32_t) &request, 0, 0);

}

void command_call_rm(char *rmCommandName){
    struct FAT32DriverState state_driver;
    // read_clusters(&state_driver.dir_table_buf, current_directory_cluster, 1);
    syscall(8, (uint32_t) &state_driver.dir_table_buf, current_directory_cluster, 1);

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
                // delete(request);
                syscall(3, (uint32_t) &request, 0, 0);
                break;      
            }
        }
    }
}

// void command_call_Whereis(char *whCommandName){
//     struct CURRENT_DIR_STACK path;
//     struct CURRENT_DIR_STACK dir;
//     struct CURRENT_DIR_STACK ret_stack;
//     syscall(9, (uint32_t) &path, 0, 0);
//     syscall(9, (uint32_t) &dir, 0, 0);
//     syscall(9, (uint32_t) &ret_stack, 0, 0);

//     // struct FAT32DriverState state_driver;
//     // read_clusters(&state_driver.dir_table_buf, current_directory_cluster, 1);
//     // read_clusters(&state_driver.fat_table, 1, 1);

//     bool isFolder = FALSE;
//     char name[8];
//     char ext[3];
//     uint8_t i = 0;
//     uint8_t j = 0;

//     while ((whCommandName[i+8] != '.') && (whCommandName[i+8] != ' ') && (i < 8)){
//         name[i] = whCommandName[i+8];
//         i++;
//     }

//     if (whCommandName[i+8] == ' '){
//         // Error karena tidak ada extensi
//         return;
//     }

//     if (whCommandName[i+8] == '\0'){
//         // berarti folder
//         isFolder = TRUE;
//         for (int z = 0; z < 3; z++){
//             ext[z] = '\0';
//         }
//     }

//     if (i < 8){
//         for (int z = i; z < 8; z++){
//             name[z] = '\0';
//         }
//     }

//     if (!isFolder){
//         j = i+1;
//         while ((whCommandName[j+3] != ' ') && (j-i-1 < 3)){
//             ext[j-i-1] = whCommandName[j+3];
//             j++;
//         }

//         if (j-i-1 == 3 && whCommandName[j+3] != '\0' && whCommandName[j+3] != ' '){
//             // Error karena extensi terlalu panjang
//             return;
//         }

//         if (j-i-1 < 3){
//             for (int z = j-i-1; z < 3; z++){
//                 ext[z] = '\0';
//             }
//         }

//         do // proses pencarian ketika dia file
//     {   
//         current_directory_cluster = syscall(10, (uint32_t) &dir, 0, 0);
//         syscall(8, (uint32_t) &state_driver.dir_table_buf, current_directory_cluster, 1);
//         for (int m = 0; m < 64; m++){
//             if (state_driver.dir_table_buf.table[m].buff == 0){
//                 syscall(11, (uint32_t) &dir, (uint32_t) current_directory_cluster, 0);
//             }

//             if (memcmp(state_driver.dir_table_buf.table[m].name, request.name, 8) == 0 && 
//                 memcmp(state_driver.dir_table_buf.table[m].ext, request.ext, 3) == 0){
//                 syscall(11, (uint32_t) &path, (uint32_t) current_directory_cluster, 0);

//                 // File ditemukan
//                 // request.buffer_size = state_driver.dir_table_buf.table[m].filesize;
//                 // // delete(request);
//                 // syscall(3, (uint32_t) &request, 0, 0);
//                 // break;      


//             }
//         }

//     } while (dir.top != 0);

//     } else {
//         do // proses pencarian ketika dia folder
//     {   
//         current_directory_cluster = syscall(10, (uint32_t) &dir, 0, 0);
//         syscall(8, (uint32_t) &state_driver.dir_table_buf, current_directory_cluster, 1);
//         for (int m = 0; m < 64; m++){

//             if (state_driver.dir_table_buf.table[m].buff == 0){
//                 syscall(11, (uint32_t) &dir, (uint32_t) current_directory_cluster, 0);
//             }

//             if (memcmp(state_driver.dir_table_buf.table[m].name, request.name, 8) == 0 && 
//                 memcmp(state_driver.dir_table_buf.table[m].ext, request.ext, 3) == 0){
//                     syscall(11, (uint32_t) &path, (uint32_t) current_directory_cluster, 0);
                
//                 // File ditemukan
//                 // request.buffer_size = state_driver.dir_table_buf.table[m].filesize;
//                 // // delete(request);
//                 // syscall(3, (uint32_t) &request, 0, 0);
//                 // break;      


//             }
//         }

//     } while (dir.top != 0);
//     }

    
    



// }

void dfs(struct CURRENT_DIR_STACK * dir, char * name, char * ext){
    struct FAT32DriverState state_driver;
    char * returnVal;
    char isFolder[3] = "\0\0\0";
    syscall(8, (uint32_t) &state_driver.dir_table_buf, current_directory_cluster, 1);
    for (int m = 1; m < 64; m++){
        if (state_driver.dir_table_buf.table[m].user_attribute == UATTR_NOT_EMPTY){
            if (memcmp(state_driver.dir_table_buf.table[m].name, name, 8) == 0 && 
                memcmp(state_driver.dir_table_buf.table[m].ext, ext, 3) == 0){
                // File ditemukan
                syscall(15, (uint32_t) &dir, (uint32_t) &returnVal, 0);
                syscall(14, (uint32_t) &dirRet, (uint32_t) returnVal, 0);
            }
        }
        if (memcmp(state_driver.dir_table_buf.table[m].name, isFolder, 3) == 0){
            syscall(11, (uint32_t) &dir, 0, (uint32_t) state_driver.dir_table_buf.table[m].name);
            dfs(dir, name, ext);
        }
    }
}

void command_call_whereis(char *whCommandName){
    struct CURRENT_DIR_STACK dir;
    
    syscall(16, (uint32_t) &dirRet, 0, 0);

    bool isFolder = FALSE;
    char name[8];
    char ext[3];
    uint8_t i = 0;
    uint8_t j = 0;

    while ((whCommandName[i+8] != '.') && (whCommandName[i+8] != ' ') && (i < 8)){
        name[i] = whCommandName[i+8];
        i++;
    }

    if (whCommandName[i+8] == ' '){
        // Error karena tidak ada extensi
        return;
    }

    if (whCommandName[i+8] == '\0'){
        // berarti folder
        isFolder = TRUE;
        for (int z = 0; z < 3; z++){
            ext[z] = '\0';
        }
    }

    if (i < 8){
        for (int z = i; z < 8; z++){
            name[z] = '\0';
        }
    }

    if (!isFolder){
        j = i+1;
        while ((whCommandName[j+3] != ' ') && (j-i-1 < 3)){
            ext[j-i-1] = whCommandName[j+3];
            j++;
        }

        if (j-i-1 == 3 && whCommandName[j+3] != '\0' && whCommandName[j+3] != ' '){
            // Error karena extensi terlalu panjang
            return;
        }

        if (j-i-1 < 3){
            for (int z = j-i-1; z < 3; z++){
                ext[z] = '\0';
            }
        }

    }

    dfs(&dir, name, ext);

}

void command_call_mv(char *path){
    struct FAT32DriverState state_driver;
    struct FAT32DriverState state_driver2;
    bool isFile = FALSE;
    syscall(8, (uint32_t) &state_driver.dir_table_buf, current_directory_cluster, 1);
    struct ClusterBuffer cbuf[4];
    struct FAT32DriverRequest request = {
        .parent_cluster_number  = current_directory_cluster,
        .buf                    = cbuf,
    };

    uint8_t i = 0;
    while (path[i] != ' ' && path[i] != '.') {
        request.name[i] = path[i];
        i++;
    }

    uint8_t ii = i;
    while (ii < 8){
        request.name[ii] = '\0';
        ii++;
    }

    if (path[i] == '.'){
        isFile = TRUE;
        i++;
        uint8_t j = 0;
        while (path[i] != ' '){
            request.ext[j] = path[i];
            i++;
            j++;
        }
    } else {
        request.ext[0] = '\0';
        request.ext[1] = '\0';
        request.ext[2] = '\0';
    }

    // mv file.txt

    uint8_t k = 0;
    char newName[8] = {0};
    char newExt[3] = {0};
    if (path[i+1] != '/'){
        // rename
        
        i++;
        uint8_t a = 0;
        while (path[i] != '\0' && path[i] != '.' && a < 8){
            newName[a] = path[i];
            i++;
            a++;
        }

        for (uint8_t z = a; z < 8; z++){
            newName[z] = '\0';
        }

        if (isFile){
            i++;
            k = 0;
            while (path[i] != '\0'){
                newExt[k] = path[i];
                i++;
                k++;
            }

            for (uint8_t z = k; z < 3; z++){
                newExt[k] = '\0';
            }
            
        } else {
            newExt[0] = '\0';
            newExt[1] = '\0';
            newExt[2] = '\0';
        }

        int m;
        uint32_t location;
        for (m = 1; m < 64; m++){
            if (state_driver.dir_table_buf.table[m].user_attribute == UATTR_NOT_EMPTY){
                if (memcmp(state_driver.dir_table_buf.table[m].name, request.name, 8) == 0 && 
                    memcmp(state_driver.dir_table_buf.table[m].ext, request.ext, 3) == 0){
                    // File ditemukan
                    request.buffer_size = state_driver.dir_table_buf.table[m].filesize;
                    // syscall(3, (uint32_t) &request, (uint32_t) &deleted, 0);
                    location = state_driver.dir_table_buf.table[m].cluster_high << 16 | state_driver.dir_table_buf.table[m].cluster_low;
                    syscall(8, (uint32_t) &state_driver2.dir_table_buf, location, 1);
                    
                    memcpy(state_driver.dir_table_buf.table[m].name, newName, 8);
                    memcpy(state_driver.dir_table_buf.table[m].ext, newExt, 3);

                    memcpy(state_driver2.dir_table_buf.table[0].name, newName, 8);
                    memcpy(state_driver2.dir_table_buf.table[0].ext, newExt, 3);

                    syscall(13, (uint32_t) &state_driver.dir_table_buf, current_directory_cluster, 1);
                    syscall(13, (uint32_t) &state_driver2.dir_table_buf, location, 1);
                    break;     
                }
            }
        }
        
    }

    else {
        // change dir
        char temporal[8];
        memcpy(temporal, state_driver.dir_table_buf.table[0].name, 8);
        // int temp = current_directory_cluster;
        // char newPath[8] = {0};
        if (path[i+2] == '.' && path[i+3] == '.'){
            uint32_t location;
            uint32_t selfLocation;
            for (int n = 1; n < 64; n++){
                if (memcmp(state_driver.dir_table_buf.table[n].name, request.name, 8) == 0 && 
                    memcmp(state_driver.dir_table_buf.table[n].ext, request.ext, 3) == 0){
                        // Folder ketemu
                        request.buffer_size = state_driver.dir_table_buf.table[n].filesize;
                        // syscall(3, (uint32_t) &request, (uint32_t) &deleted, 0);
                        selfLocation = state_driver.dir_table_buf.table[n].cluster_high << 16 | state_driver.dir_table_buf.table[n].cluster_low;
                        memcpy(state_driver.dir_table_buf.table[n].name, "\0\0\0\0\0\0\0\0", 8);
                        memcpy(state_driver.dir_table_buf.table[n].ext, "\0\0\0", 3);
                        state_driver.dir_table_buf.table[n].filesize = 0;
                        state_driver.dir_table_buf.table[n].attribute = 0;
                        state_driver.dir_table_buf.table[n].user_attribute = 0;
                        state_driver.dir_table_buf.table[n].cluster_high = 0;
                        state_driver.dir_table_buf.table[n].cluster_low = 0;
                        syscall(13, (uint32_t) &state_driver.dir_table_buf, current_directory_cluster, 1);
                        
                        break;
                        
                    }
            }
            command_call_cd("..");
            syscall(8, (uint32_t) &state_driver2.dir_table_buf, current_directory_cluster, 1);
            location = state_driver2.dir_table_buf.table[0].cluster_high << 16 | state_driver2.dir_table_buf.table[0].cluster_low;
            
            for (int o = 1; o < 64; o++){
                if (state_driver2.dir_table_buf.table[o].user_attribute != UATTR_NOT_EMPTY){
                    memcpy(state_driver2.dir_table_buf.table[o].name, request.name, 8);
                    memcpy(state_driver2.dir_table_buf.table[o].ext, request.ext, 3);
                    state_driver2.dir_table_buf.table[o].filesize = request.buffer_size;
                    state_driver2.dir_table_buf.table[o].attribute = 0;
                    state_driver2.dir_table_buf.table[o].user_attribute = UATTR_NOT_EMPTY;
                    state_driver2.dir_table_buf.table[o].cluster_high = 0;
                    state_driver2.dir_table_buf.table[o].cluster_low = selfLocation;
                    syscall(13, (uint32_t) &state_driver2.dir_table_buf, location, 1);
                    break;
                }
            }
            
            
            command_call_cd(temporal);
            
        }

        else {
            uint8_t kk = 0;
            char newPath[8] = {0};
            i+=2;
            while (path[i] != '\0'){
                newPath[kk] = path[i];
                i++;
                kk++;
            }
            int m;
            uint32_t location;
            uint32_t selfLocation;
            for (m = 1; m < 64; m++){
                if (state_driver.dir_table_buf.table[m].user_attribute == UATTR_NOT_EMPTY){
                    if (memcmp(state_driver.dir_table_buf.table[m].name, newPath, 8) == 0 && 
                        memcmp(state_driver.dir_table_buf.table[m].ext, "\0\0\0", 3) == 0){
                            for (int n = 1; n < 64; n++){
                                if (memcmp(state_driver.dir_table_buf.table[n].name, request.name, 8) == 0 && 
                                    memcmp(state_driver.dir_table_buf.table[n].ext, request.ext, 3) == 0){
                                        // Folder ketemu
                                        request.buffer_size = state_driver.dir_table_buf.table[n].filesize;
                                        // syscall(3, (uint32_t) &request, (uint32_t) &deleted, 0);
                                        location = state_driver.dir_table_buf.table[m].cluster_high << 16 | state_driver.dir_table_buf.table[m].cluster_low;
                                        selfLocation = state_driver.dir_table_buf.table[n].cluster_high << 16 | state_driver.dir_table_buf.table[n].cluster_low;
                                        request.parent_cluster_number = location;

                                        syscall(8, (uint32_t) &state_driver2.dir_table_buf, location, 1);
                                        
                                        // syscall(2, (uint32_t) &request, 0, 0);
                                        for (int o = 1; o < 64; o++){
                                            if (state_driver2.dir_table_buf.table[o].user_attribute != UATTR_NOT_EMPTY){
                                                memcpy(state_driver2.dir_table_buf.table[o].name, request.name, 8);
                                                memcpy(state_driver2.dir_table_buf.table[o].ext, request.ext, 3);
                                                state_driver2.dir_table_buf.table[o].filesize = request.buffer_size;
                                                state_driver2.dir_table_buf.table[o].attribute = 0;
                                                state_driver2.dir_table_buf.table[o].user_attribute = UATTR_NOT_EMPTY;
                                                state_driver2.dir_table_buf.table[o].cluster_high = 0;
                                                state_driver2.dir_table_buf.table[o].cluster_low = selfLocation;
                                                syscall(13, (uint32_t) &state_driver2.dir_table_buf, location, 1);
                                                break;
                                            }
                                        }

                                        memcpy(state_driver.dir_table_buf.table[n].name, "\0\0\0\0\0\0\0\0", 8);
                                        memcpy(state_driver.dir_table_buf.table[n].ext, "\0\0\0", 3);
                                        state_driver.dir_table_buf.table[n].filesize = 0;
                                        state_driver.dir_table_buf.table[n].attribute = 0;
                                        state_driver.dir_table_buf.table[n].user_attribute = 0;
                                        state_driver.dir_table_buf.table[n].cluster_high = 0;
                                        state_driver.dir_table_buf.table[n].cluster_low = 0;
                                        syscall(13, (uint32_t) &state_driver.dir_table_buf, current_directory_cluster, 1);
                                        break; 
                                }    
                            }
                    }
                }
            }
        }
    }
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

    if (!initialized_cd_stack){
        initialized_cd_stack = TRUE;
        // init_current_dir_stack(&current_dir_stack);
        syscall(9, (uint32_t) &current_dir_stack, 0, 0);
    }
    
    while (TRUE) {
        char * cwdpath = dirStackConverter(&current_dir_stack);
        syscall(4, (uint32_t) buf, (uint32_t) cwdpath, (uint32_t) row_shell);
        row_shell++;
        command = getCommandInput((char*) buf, 16);
        
        // TODO: prosess
        switch (command){
            case 0:
                // cd
                command_call_multi_cd(((char *) buf) + 3);
                break;
            case 1:
                // ls -a
                command_call_ls(4);
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
                command_call_cp((char *) buf);
                break;
            case 5:
                // rm
                command_call_rm((char *) buf);
                break;
            case 6:
                // mv
                command_call_mv(((char *) buf) + 3);
                break;
            case 7:
                // whereis
                command_call_whereis((char *) buf);
                break;
            case 8:
                // ls -ff
                command_call_ls(3);
                break;
            case 9:
                // ls -F
                command_call_ls(1);
                break;
            case 10:
                // ls -f
                command_call_ls(2);
                break;
            case 11:
                // ls
                command_call_ls(0);
                break;
            case 12:
                // cls
                row_shell = 0;
                syscall(17, (uint32_t) buf, (uint32_t) cwdpath, (uint32_t) row_shell);
            default:
                // command not found
                // framebuffer_write(0, 79, 'x', 0x0f, 0);
                break;
        }

        syscall(5, (uint32_t) buf, 0x20, 0xF);
    }

    return 0;
}
