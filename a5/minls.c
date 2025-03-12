#include "mincommon.h"
void printInodeDirs(uint32_t ind, Args_t *args, size_t zone_size, 
                    intptr_t partition_addr, size_t block_size);
void printZone(Args_t *args, intptr_t zone_addr, size_t zone_size, 
                uint32_t num_bytes);
void getPerms(Inode_t *inode, char *buffer);
void printFileInfo(Inode_t *inode, char *name);
void printPath(Args_t *args);
void getFilePath(Args_t *args, char *name);

Inode_t *inodes;

int main(int argc, char *argv[]) {
    Args_t args;
    PartitionTableEntry_t part_table;
    SuperBlock_t super_blk;

    parseArgs(argc, argv, MINLS_BOOL, &args);
    parsePartitionTable(&args, &part_table);
    parseSuperBlock(&args, &part_table, &super_blk);

    size_t part_addr = part_table.lFirst * SECTOR_SIZE;    // First sector
    size_t zone_size = super_blk.blocksize << super_blk.log_zone_size;

    // Get Inodes
    inodes = (Inode_t *) malloc(sizeof(Inode_t) * super_blk.i_blocks);
    size_t inode_addr = part_addr + 
                            ((2 + super_blk.i_blocks + super_blk.z_blocks) *
                            super_blk.blocksize);

    fseek(args.image, inode_addr, SEEK_SET);    // Go to inode 1 address
    fread(inodes, sizeof(Inode_t), super_blk.ninodes, args.image);
    uint32_t found_inode_ind = findInode(&args, zone_size, part_addr, 
                                            super_blk.blocksize);
    // printf("Found inode number: %d\n", found_inode_ind);

    if (!found_inode_ind) {
        perror("Error: File not found\n");
        exit(EXIT_FAILURE);
    }

    Inode_t *found_inode = inodes + found_inode_ind - 1;
    char file_name[MAX_NAME_SIZE];
    getFilePath(&args, file_name);
    if (found_inode->mode & DIRECTORY) {
        printf("/%s:\n", file_name);
        printInodeDirs(found_inode_ind, &args, zone_size, part_addr, 
                    super_blk.blocksize);
    } else {
        printFileInfo(found_inode, file_name);
    }
}

void getFilePath(Args_t *args, char *name) {
    char path_copy[MAX_PATH_SIZE];
    strcpy(path_copy, args->image_path);
    char *path_token = strtok(path_copy, "/");

    while (path_token) {
        strcat(name, path_token);
        path_token = strtok(NULL, "/");
        if (path_token) strcat(name, "/");
    }
}

void printInodeDirs(uint32_t ind, Args_t *args, size_t zone_size, 
                    intptr_t partition_addr, size_t block_size) {
    // printf("Entered findInode with path: %s\n", args->image_path);

    uint32_t curr_inode_ind = ind - 1;
    Inode_t *curr_inode = inodes + curr_inode_ind;

    uint32_t indirect_zones[INDIRECT_ZONES];
    uint32_t double_zones[INDIRECT_ZONES];

    int i;
    uint32_t bytes_left = curr_inode->size;

    for (i = 0;  i < DIRECT_ZONES && bytes_left > 0; i++) {
        // printf("Entered Direct Zones\n");
        uint32_t curr_zone = curr_inode->zone[i];
        uint32_t num_bytes = zone_size;

        // if number of bytes left is less than the size of zone
        if (bytes_left < zone_size) {
            // number of bytes to read should be bytes left
            num_bytes = bytes_left;
        }

        // check if deleted zone
        if (curr_zone == 0) {
            // decrement number of bytes left to read
            bytes_left -= num_bytes;
            continue;
        }

        // seek/read num_bytes at zone address
        intptr_t zone_addr = partition_addr + (curr_zone * zone_size);
        printZone(args, zone_addr, zone_size, num_bytes);
        bytes_left -= num_bytes;
    }

    //TODO: indirect zones
    if (bytes_left > 0) {
        if (curr_inode->indirect == 0) {
            // TODO: huh??
        } else {
            // printf("Entered Indirect Zones\n");
            intptr_t indirect_addr = partition_addr + 
                                        (curr_inode->indirect * zone_size);
            fseek(args->image, indirect_addr, SEEK_SET);
            fread(indirect_zones, sizeof(uint32_t), 
                    INDIRECT_ZONES, args->image);

            for (i = 0;  i < INDIRECT_ZONES && bytes_left > 0; i++) {
                // printf("Entered Direct Zones\n");
                uint32_t curr_zone = indirect_zones[i];
                // printf("curr_zone: %d\n", curr_zone);
                uint32_t num_bytes = block_size; // TODO: why block size

                // if number of bytes left is less than the size of zone
                if (bytes_left < block_size) {
                    // number of bytes to read should be bytes left
                    num_bytes = bytes_left;
                }

                // check if deleted zone
                if (curr_zone == 0) {
                    // decrement number of bytes left to read
                    bytes_left -= num_bytes;
                    continue;
                }

                // seek/read num_bytes at zone address
                intptr_t zone_addr = partition_addr + 
                                        (curr_zone * zone_size);
                printZone(args, zone_addr, zone_size, num_bytes);
                bytes_left -= num_bytes;
            }
        }
    }

    //TODO: double indirect zones
}

void printZone(Args_t *args, intptr_t zone_addr, size_t zone_size, 
                uint32_t num_bytes) {
    uint8_t zone_buff[zone_size];
    int j;
    // seek/read num_bytes at zone address
    fseek(args->image, zone_addr, SEEK_SET);
    fread(zone_buff, sizeof(uint8_t), num_bytes, args->image);

    // iterate through directory entries in zone
    uint32_t num_dirs = num_bytes / sizeof(DirEntry_t);
    for (j = 0; j < num_dirs; j++) {
        // index into zone_buff to access current directory entry
        DirEntry_t *curr_dir = (DirEntry_t*) zone_buff + j;

        if (curr_dir->inode == 0) { // directory deleted
            continue;
        } else { 
            printFileInfo(inodes + curr_dir->inode - 1, curr_dir->name);
        }
    }
}

void printFileInfo(Inode_t *inode, char *name) {
    char bit_buffer[10];
    
    getPerms(inode, bit_buffer);
    printf("%s", bit_buffer);
    printf("%*d", 9, inode->size);
    printf(" %s\n", name);

}

void getPerms(Inode_t *inode, char *buffer) {
    // Print out perms for given inode
    buffer[10] = '\0';  // Add null terminator

    if (inode->mode & REGULAR_FILE) {
        buffer[0] = '-';
    } else if (inode->mode & DIRECTORY) {
        buffer[0] = 'd';
    }

    if (inode->mode & OWNER_READ) { 
        buffer[1] = 'r'; 
    } else { 
        buffer[1] = '-'; 
    }

    if (inode->mode & OWNER_WRITE) { 
        buffer[2] = 'w'; 
    } else { 
        buffer[2] = '-'; 
    }

    if (inode->mode & OWNER_EXEC) { 
        buffer[3] = 'x'; 
    } else { 
        buffer[3] = '-'; 
    }

    if (inode->mode & GROUP_READ) { 
        buffer[4] = 'r'; 
    } else { 
        buffer[4] = '-'; 
    }

    if (inode->mode & GROUP_WRITE) { 
        buffer[5] = 'w'; 
    } else { 
        buffer[5] = '-'; 
    }

    if (inode->mode & GROUP_EXEC) { 
        buffer[6] = 'x'; 
    } else { 
        buffer[6] = '-'; 
    }

    if (inode->mode & GROUP_READ) { 
        buffer[7] = 'r'; 
    } else { 
        buffer[7] = '-'; 
    }

    if (inode->mode & GROUP_WRITE) { 
        buffer[8] = 'w'; 
    } else { 
        buffer[8] = '-'; 
    }

    if (inode->mode & GROUP_EXEC) { 
        buffer[9] = 'x'; 
    } else { 
        buffer[9] = '-'; 
    }
}