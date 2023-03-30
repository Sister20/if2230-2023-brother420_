#include "lib-header/stdtype.h"
#include "lib-header/fat32.h"
#include "lib-header/stdmem.h"

// static struct ClusterBuffer clusterBuffer;
// static struct FAT32FileAllocationTable fat_table;
// static struct FAT32DirectoryEntry root_dir_entry;
// static struct FAT32DirectoryTable root_dir_table;
struct FAT32DriverState driver_state;

// static struct FAT32DriverRequest driver_request;

const uint8_t fs_signature[BLOCK_SIZE] = {
    'C', 'o', 'u', 'r', 's', 'e', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',  ' ',
    'D', 'e', 's', 'i', 'g', 'n', 'e', 'd', ' ', 'b', 'y', ' ', ' ', ' ', ' ',  ' ',
    'L', 'a', 'b', ' ', 'S', 'i', 's', 't', 'e', 'r', ' ', 'I', 'T', 'B', ' ',  ' ',
    'M', 'a', 'd', 'e', ' ', 'w', 'i', 't', 'h', ' ', '<', '3', ' ', ' ', ' ',  ' ',
    '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '2', '0', '2', '3', '\n',
    [BLOCK_SIZE-200] = 'F',
    [BLOCK_SIZE-199] = 'A',
    [BLOCK_SIZE-198] = 'C',
    [BLOCK_SIZE-197] = 'T',
    [BLOCK_SIZE-196] = '{',
    [BLOCK_SIZE-195] = 'c',
    [BLOCK_SIZE-194] = 'h',
    [BLOCK_SIZE-193] = '1',
    [BLOCK_SIZE-192] = '2',
    [BLOCK_SIZE-191] = 'u',
    [BLOCK_SIZE-190] = 'r',
    [BLOCK_SIZE-189] = 'u',
    [BLOCK_SIZE-188] = '_',
    [BLOCK_SIZE-187] = '1',
    [BLOCK_SIZE-186] = '5',
    [BLOCK_SIZE-185] = '_',
    [BLOCK_SIZE-184] = 'b',
    [BLOCK_SIZE-183] = '3',
    [BLOCK_SIZE-182] = '5',
    [BLOCK_SIZE-181] = 't',
    [BLOCK_SIZE-180] = '_',
    [BLOCK_SIZE-179] = '6',
    [BLOCK_SIZE-178] = '1',
    [BLOCK_SIZE-177] = 'r',
    [BLOCK_SIZE-176] = 'l',
    [BLOCK_SIZE-175] = ':',
    [BLOCK_SIZE-174] = 'D',
    [BLOCK_SIZE-173] = '}',
    [BLOCK_SIZE-2] = 'O',
    [BLOCK_SIZE-1] = 'k',
};

/* -- Driver Interfaces -- */

/**
 * Convert cluster number to logical block address
 * 
 * @param cluster Cluster number to convert
 * @return uint32_t Logical Block Address
 */
uint32_t cluster_to_lba(uint32_t cluster){
    return (cluster - 2) * CLUSTER_SIZE + 1;
}

/**
 * Initialize DirectoryTable value with parent DirectoryEntry and directory name
 * 
 * @param dir_table          Pointer to directory table
 * @param name               8-byte char for directory name
 * @param parent_dir_cluster Parent directory cluster number
 */
void init_directory_table(struct FAT32DirectoryTable *dir_table, char *name, uint32_t parent_dir_cluster){
    // dir_table->table[0].name[0] = '.';
    dir_table->table[0].cluster_high = (uint16_t)(parent_dir_cluster >> 16);
    dir_table->table[0].cluster_low = (uint16_t)(parent_dir_cluster & 0xFFFF);
    for (uint8_t i = 0; i < 8; i++){
        dir_table->table[0].name[i] = name[i];
    }
    dir_table->table[0].user_attribute = UATTR_NOT_EMPTY;
    

    // dir_table->table[1].name[0] = '.';
    // dir_table->table[1].name[1] = '.';
    // dir_table->table[1].cluster_high = (uint16_t)(parent_dir_cluster >> 16);
    // dir_table->table[1].cluster_low = (uint16_t)(parent_dir_cluster & 0xFFFF);
    // dir_table->table[2].cluster_high = 0;
    // dir_table->table[2].cluster_low = 2;
}

/**
 * Checking whether filesystem signature is missing or not in boot sector
 * 
 * @return True if memcmp(boot_sector, fs_signature) returning inequality
 */
bool is_empty_storage(void){
    uint8_t boot_sector[BLOCK_SIZE];
    read_blocks(boot_sector, 0, 1);
    return memcmp(boot_sector, fs_signature, BLOCK_SIZE);
}

/**
 * Create new FAT32 file system. Will write fs_signature into boot sector and 
 * proper FileAllocationTable (contain CLUSTER_0_VALUE, CLUSTER_1_VALUE, 
 * and initialized root directory) into cluster number 1
 */
void create_fat32(void){
    uint8_t boot_sector[BLOCK_SIZE];
    memset(boot_sector, 0, BLOCK_SIZE);
    memcpy(boot_sector, fs_signature, BLOCK_SIZE);
    write_blocks(boot_sector, 0, 1);

    driver_state.fat_table.cluster_map[0] = CLUSTER_0_VALUE;
    driver_state.fat_table.cluster_map[1] = CLUSTER_1_VALUE;
    driver_state.fat_table.cluster_map[2] = FAT32_FAT_END_OF_FILE;
    write_clusters(driver_state.fat_table.cluster_map, 1, 1);
    
    struct  FAT32DirectoryTable root_dir_table = {0};
    init_directory_table(&root_dir_table, "ROOT", 2);
    write_clusters(&root_dir_table.table, 2, 1);
    
}

/**
 * Initialize file system driver state, if is_empty_storage() then create_fat32()
 * Else, read and cache entire FileAllocationTable (located at cluster number 1) into driver state
 */
void initialize_filesystem_fat32(void){
    if (is_empty_storage()){
        create_fat32();
    } else {
        create_fat32();
        read_clusters(driver_state.fat_table.cluster_map, 1, 1);
    }
}

/**
 * Write cluster operation, wrapper for write_blocks().
 * Recommended to use struct ClusterBuffer
 * 
 * @param ptr            Pointer to source data
 * @param cluster_number Cluster number to write
 * @param cluster_count  Cluster count to write, due limitation of write_blocks block_count 255 => max cluster_count = 63
 */
void write_clusters(const void *ptr, uint32_t cluster_number, uint8_t cluster_count){
    write_blocks(ptr, cluster_number * 4, cluster_count * CLUSTER_BLOCK_COUNT);
}

/**
 * Read cluster operation, wrapper for read_blocks().
 * Recommended to use struct ClusterBuffer
 * 
 * @param ptr            Pointer to buffer for reading
 * @param cluster_number Cluster number to read
 * @param cluster_count  Cluster count to read, due limitation of read_blocks block_count 255 => max cluster_count = 63
 */
void read_clusters(void *ptr, uint32_t cluster_number, uint8_t cluster_count){
    read_blocks(ptr, cluster_number * 4, cluster_count * CLUSTER_BLOCK_COUNT);
}





/* -- CRUD Operation -- */

/**
 *  FAT32 Folder / Directory read
 *
 * @param request buf point to struct FAT32DirectoryTable,
 *                name is directory name,
 *                ext is unused,
 *                parent_cluster_number is target directory table to read,
 *                buffer_size must be exactly sizeof(struct FAT32DirectoryTable)
 * @return Error code: 0 success - 1 not a folder - 2 not found - -1 unknown
 */

int8_t read_directory(struct FAT32DriverRequest request){
    uint32_t location;
    read_clusters(&driver_state.dir_table_buf, request.parent_cluster_number, 1);
    for (int i = 0; i < 64; i++){
        if (memcmp(driver_state.dir_table_buf.table[i].name, request.name, 8) == 0){
            if (request.buffer_size < driver_state.dir_table_buf.table[i].filesize) {
                return 2;
            } else if (driver_state.dir_table_buf.table[i].attribute == 1){
                return 1;
            }  
            location = (driver_state.dir_table_buf.table[i].cluster_high << 16) | driver_state.dir_table_buf.table[i].cluster_low;
            read_clusters(request.buf + CLUSTER_SIZE, location, 1);
            break;
        }
    }
    return 0;
}


/**
 * FAT32 read, read a file from file system.
 *
 * @param request All attribute will be used for read, buffer_size will limit reading count
 * @return Error code: 0 success - 1 not a file - 2 not enough buffer - 3 not found - -1 unknown
 */
int8_t read(struct FAT32DriverRequest request){
    uint32_t total_cluster;
    uint32_t location;
    read_clusters(&driver_state.dir_table_buf, request.parent_cluster_number, 1);
    read_clusters(&driver_state.fat_table, 1, 1);
    for (int i = 0; i < 64; i++){
        if (memcmp(driver_state.dir_table_buf.table[i].name, request.name, 8) == 0 && memcmp(driver_state.dir_table_buf.table[i].ext, request.ext, 3) == 0){
            if (request.buffer_size < driver_state.dir_table_buf.table[i].filesize) {
                return 2;
            } else if (driver_state.dir_table_buf.table[i].attribute == 1){
                return 1;
            }   
            total_cluster = driver_state.dir_table_buf.table[i].filesize / CLUSTER_SIZE;
            if (total_cluster * CLUSTER_SIZE < driver_state.dir_table_buf.table[i].filesize){
                total_cluster += 1;
            } else {
                location = (driver_state.dir_table_buf.table[i].cluster_high << 16) | driver_state.dir_table_buf.table[i].cluster_low;
                for (uint32_t j = 0; j < total_cluster; j++){
                    if (j == 0){
                        read_clusters(request.buf + CLUSTER_SIZE * j, location, 1);
                    } else {
                        read_clusters(request.buf + CLUSTER_SIZE * j, driver_state.fat_table.cluster_map[location], 1);
                        location = driver_state.fat_table.cluster_map[location];
                    }
                }
            }
            break;
        }
    

    }
    return 0;
}


/**
 * FAT32 write, write a file or folder to file system.
 *
 * @param request All attribute will be used for write, buffer_size == 0 then create a folder / directory
 * @return Error code: 0 success - 1 file/folder already exist - 2 invalid parent cluster - -1 unknown
 */
int8_t write(struct FAT32DriverRequest request){
    read_clusters(&driver_state.dir_table_buf, request.parent_cluster_number, 1);
    read_clusters(&driver_state.fat_table, 1, 1);
    if (driver_state.fat_table.cluster_map[request.parent_cluster_number] != FAT32_FAT_END_OF_FILE){
        return 2;
    }
    uint16_t cluster = 0;
    uint16_t clusterNext = 0;
    uint16_t cluster_table = 0;
    while (driver_state.fat_table.cluster_map[cluster] != 0){
        cluster++; // Ini index
    }
    if (request.buffer_size == 0){
        
        for (int i = 0; i < 64; i++){
            if (memcmp(driver_state.dir_table_buf.table[i].name, request.name, 8) == 0){
                return 1;
            }
        }
        
        driver_state.fat_table.cluster_map[cluster] = FAT32_FAT_END_OF_FILE;
        write_clusters(driver_state.fat_table.cluster_map,1,1);

        struct FAT32DirectoryTable new_dir = {0};
        init_directory_table(&new_dir, request.name, cluster);
        
        write_clusters(new_dir.table, cluster, 1);

        cluster_table = 0;
    
        // cari empty di dirtable
        while (driver_state.dir_table_buf.table[cluster_table].user_attribute == UATTR_NOT_EMPTY){
            cluster_table++; // Ini index
        }
        
        driver_state.dir_table_buf.table[cluster_table] = new_dir.table[0];
        write_clusters(driver_state.dir_table_buf.table, request.parent_cluster_number,1);
        
        

    } else if (request.buffer_size > 0){
        
        for (int i = 0; i < 64; i++){
            if (memcmp(driver_state.dir_table_buf.table[i].name, request.name, 8) == 0 && 
                memcmp(driver_state.dir_table_buf.table[i].ext, request.ext, 3) == 0){
                return 1;
            }
        }
        
        uint32_t total_cluster = request.buffer_size / CLUSTER_SIZE;
        if (total_cluster * CLUSTER_SIZE < request.buffer_size){
            total_cluster += 1;
        }
        
        cluster_table = 0;
        
        while (driver_state.dir_table_buf.table[cluster_table].user_attribute == UATTR_NOT_EMPTY){
            cluster_table++; // Ini index
        }

        memcpy(driver_state.dir_table_buf.table[cluster_table].name, request.name, 8);
        memcpy(driver_state.dir_table_buf.table[cluster_table].ext, request.ext, 3);
        driver_state.dir_table_buf.table[cluster_table].attribute = 0;
        driver_state.dir_table_buf.table[cluster_table].user_attribute = UATTR_NOT_EMPTY;
        driver_state.dir_table_buf.table[cluster_table].cluster_low = cluster;
        for (uint32_t i = 0; i < total_cluster + 1; i++){
            
            cluster = 0;

            while (driver_state.fat_table.cluster_map[cluster] != 0){
                cluster++; // Ini index
            }

            clusterNext = cluster + 1;

            while (driver_state.fat_table.cluster_map[clusterNext] != 0){
                clusterNext++; // Ini index
            }

            if (i < total_cluster - 1){
                driver_state.fat_table.cluster_map[cluster] = clusterNext;
                driver_state.dir_table_buf.table[cluster_table].filesize += 2048;
            } else {
                driver_state.fat_table.cluster_map[cluster] = FAT32_FAT_END_OF_FILE;
                driver_state.dir_table_buf.table[cluster_table].filesize += request.buffer_size - (CLUSTER_SIZE * i);
            }
            
            write_clusters(request.buf + CLUSTER_SIZE * i, cluster, 1);
            write_clusters(driver_state.fat_table.cluster_map,1,1);
        }

        write_clusters(driver_state.dir_table_buf.table, request.parent_cluster_number,1);
        
    }

    return 0;    
}


/**
 * FAT32 delete, delete a file or empty directory (only 1 DirectoryEntry) in file system.
 *
 * @param request buf and buffer_size is unused
 * @return Error code: 0 success - 1 not found - 2 folder is not empty - -1 unknown
 */
int8_t delete(struct FAT32DriverRequest request){
    struct FAT32DirectoryEntry temp_ent = {0};
    struct FAT32DirectoryTable temp_tab = {0};
    uint32_t location = 0;
    uint16_t cluster_temp = 0;
    bool flag = 0;
    int index = 0;
    read_clusters(&driver_state.dir_table_buf, request.parent_cluster_number, 1);
    read_clusters(&driver_state.fat_table, 1, 1);
    temp_tab = driver_state.dir_table_buf;
    if (request.buffer_size == 0){
        for (int i = 0; i < 64; i++){
            if (memcmp(driver_state.dir_table_buf.table[i].name, request.name, 8) == 0){
                location = (driver_state.dir_table_buf.table[i].cluster_high << 16) | driver_state.dir_table_buf.table[i].cluster_low;
                index = i;
                break;
            }
        }
        read_clusters(&temp_tab.table, location, 1);
        for (int i = 1; i < 64; i++){
            if (temp_tab.table[i].user_attribute == UATTR_NOT_EMPTY){
                return 2;
            }
        }
        if (memcmp(request.name, "ROOT", 8) == 0){
            return -1;
        }
        driver_state.dir_table_buf.table[index] = temp_ent;
        driver_state.fat_table.cluster_map[location] = 0;
        // for (int i = 0; i < 64; i++){
        //     if (memcmp(driver_state.dir_table_buf.table[i].name, request.name, 8) == 0){
        //         location = (driver_state.dir_table_buf.table[i].cluster_high << 16) | driver_state.dir_table_buf.table[i].cluster_low;
                
        //         break;
        //     }
        // }
        write_clusters(driver_state.fat_table.cluster_map,1,1);
        write_clusters(driver_state.dir_table_buf.table, request.parent_cluster_number,1);

        return 0;
        // delete folder
    } else {
    for (int i = 0; i < 64; i++){
        if (memcmp(driver_state.dir_table_buf.table[i].name, request.name, 8) == 0 && memcmp(driver_state.dir_table_buf.table[i].ext, request.ext, 3) == 0){
            location = (driver_state.dir_table_buf.table[i].cluster_high << 16) | driver_state.dir_table_buf.table[i].cluster_low;
            driver_state.dir_table_buf.table[i] = temp_ent;
            do
            {
                if (driver_state.fat_table.cluster_map[location] == FAT32_FAT_END_OF_FILE){
                    driver_state.fat_table.cluster_map[location] = 0;
                    flag = 1;
                } else {
                cluster_temp = location;
                location = driver_state.fat_table.cluster_map[location];
                driver_state.fat_table.cluster_map[cluster_temp] = 0;
                }
            } while (!flag);
            write_clusters(driver_state.fat_table.cluster_map,1,1);
            write_clusters(driver_state.dir_table_buf.table, request.parent_cluster_number,1);
            return 0;
        }
    }
    return 1;
    }
}