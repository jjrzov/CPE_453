#include "mincommon.h"

void printInodeFileContents(uint32_t ind, Args_t *args, size_t zone_size, 
                    intptr_t partition_addr, size_t block_size);

Inode_t *inodes;

int main(int argc, char *argv[]) {
    Args_t args;
    PartitionTableEntry_t part_table;
    SuperBlock_t super_blk;

    // Parse arguments and partition table/super block
    parseArgs(argc, argv, !MINLS_BOOL, &args);
    parsePartitionTable(&args, &part_table);
    parseSuperBlock(&args, &part_table, &super_blk);

    // Calculate partition address and zone size
    size_t part_addr = part_table.lFirst * SECTOR_SIZE;    // First sector
    size_t zone_size = super_blk.blocksize << super_blk.log_zone_size;

    // Malloc list of inodes
    inodes = (Inode_t *) malloc(sizeof(Inode_t) * super_blk.ninodes);
    size_t inode_addr = part_addr + 
                            ((2 + super_blk.i_blocks + super_blk.z_blocks) *
                            super_blk.blocksize);

    // Seek and read inode list from image
    fseek(args.image, inode_addr, SEEK_SET);    // Go to inode 1 address
    fread(inodes, sizeof(Inode_t), super_blk.ninodes, args.image);

    // Find inode corresponding to path
    uint32_t found_inode_ind = findInode(args.src_path, &args, zone_size, 
                                            part_addr, zone_size);
    
    // If inode not found, file does not exist
    if (!found_inode_ind) {
        perror("Error: File not found\n");
        exit(EXIT_FAILURE);
    } 

    // Traverse inode list for found inode
    Inode_t *found_inode = inodes + found_inode_ind - 1;

    // Found inode must be a regular file
    if ((found_inode->mode & FILE_TYPE_MASK) != REGULAR_FILE) {
        perror("Error: Not a file\n");
        exit(EXIT_FAILURE);
    }

    // Write contents of file to dest
    printInodeFileContents(found_inode_ind, &args, zone_size, part_addr, 
                    zone_size);
}

/**
 * Traverses through zones of given inode index and writes src file to dest
 */
void printInodeFileContents(uint32_t ind, Args_t *args, size_t zone_size, 
                    intptr_t partition_addr, size_t block_size) {
    uint32_t curr_inode_ind = ind - 1;
    Inode_t *curr_inode = inodes + curr_inode_ind;

    uint8_t zone_buff[zone_size];
    uint32_t indirect_zones[INDIRECT_ZONES];
    uint32_t double_zones[INDIRECT_ZONES];

    int i;
    uint32_t bytes_left = curr_inode->size;

    for (i = 0;  i < DIRECT_ZONES && bytes_left > 0; i++) {
        uint32_t curr_zone = curr_inode->zone[i];
        uint32_t num_bytes = zone_size;

        // if number of bytes left is less than the size of zone
        if (bytes_left < zone_size) {
            // number of bytes to read should be bytes left
            num_bytes = bytes_left;
        }

        // check if empty zone
        if (curr_zone == 0) {
            memset(zone_buff, 0, num_bytes);
            fwrite(zone_buff, sizeof(char), num_bytes, args->dest);
            // decrement number of bytes left to read
            bytes_left -= num_bytes;
            continue;
        }

        // seek/read num_bytes at zone address
        intptr_t zone_addr = partition_addr + (curr_zone * zone_size);
        fseek(args->image, zone_addr, SEEK_SET);
        fread(zone_buff, sizeof(uint8_t), num_bytes, args->image);
        fwrite(zone_buff, sizeof(char), num_bytes, args->dest);
        bytes_left -= num_bytes;
    }

    if (bytes_left > 0) {
        if (curr_inode->indirect == 0) {
            // Deal with holes
            uint32_t hole_bytes = (INDIRECT_ZONES * block_size);    
            
            uint32_t read_bytes = hole_bytes;
            if (bytes_left < hole_bytes) {
                read_bytes -= bytes_left;
            }

            memset(zone_buff, 0, block_size);

            // Get amount of blocks
            uint32_t num_blocks = read_bytes / block_size;
            for (i = 0; i < num_blocks; i++) {
                fwrite(zone_buff, sizeof(char), block_size, args->dest);
            }

            bytes_left -= read_bytes;

        } else {
            intptr_t indirect_addr = partition_addr + 
                                        (curr_inode->indirect * zone_size);
            fseek(args->image, indirect_addr, SEEK_SET);
            fread(indirect_zones, sizeof(uint32_t), 
                    INDIRECT_ZONES, args->image);

            for (i = 0;  i < INDIRECT_ZONES && bytes_left > 0; i++) {
                uint32_t curr_zone = indirect_zones[i];
                uint32_t num_bytes = block_size;

                // if number of bytes left is less than the size of zone
                if (bytes_left < block_size) {
                    // number of bytes to read should be bytes left
                    num_bytes = bytes_left;
                }

                // check if empty zone
                if (curr_zone == 0) {
                    memset(zone_buff, 0, num_bytes);
                    fwrite(zone_buff, sizeof(char), num_bytes, args->dest);
                    // decrement number of bytes left to read
                    bytes_left -= num_bytes;
                    continue;
                }

                // seek/read num_bytes at zone address
                intptr_t zone_addr = partition_addr + (curr_zone * zone_size);
                fseek(args->image, zone_addr, SEEK_SET);
                fread(zone_buff, sizeof(uint8_t), num_bytes, args->image);
                fwrite(zone_buff, sizeof(char), num_bytes, args->dest);
                bytes_left -= num_bytes;
            }
        }
    }

    if (bytes_left > 0) {
        if (curr_inode->two_indirect == 0) {
            // Hole detected => zones referred to are to be treated as zero
            uint32_t hole_bytes = (INDIRECT_ZONES * INDIRECT_ZONES * 
                                    block_size);    
            
            uint32_t read_bytes = hole_bytes;
            if (bytes_left < hole_bytes) {
                read_bytes = bytes_left;
            }

            memset(zone_buff, 0, block_size);

            // Get amount of blocks
            uint32_t num_blocks = read_bytes / block_size;
            for (i = 0; i < num_blocks; i++) {
                fwrite(zone_buff, sizeof(char), block_size, args->dest);
            }

            bytes_left -= read_bytes;

        } else {
            intptr_t duo_indirect_addr = partition_addr + 
                                    (curr_inode->two_indirect * zone_size);
            fseek(args->image, duo_indirect_addr, SEEK_SET);
            fread(double_zones, sizeof(uint32_t), INDIRECT_ZONES, args->image);

            for (i = 0;  i < INDIRECT_ZONES; i++) {
                uint32_t curr_indirect_zone = double_zones[i];

                if (curr_indirect_zone == 0) {
                    // Hole within the indirect zone
                    uint32_t hole_bytes = (INDIRECT_ZONES * block_size);    
            
                    uint32_t read_bytes = hole_bytes;
                    if (bytes_left < hole_bytes) {
                        read_bytes = bytes_left;
                    }

                    memset(zone_buff, 0, block_size);

                    // Get amount of blocks
                    uint32_t num_blocks = read_bytes / block_size;
                    for (int j = 0; j < num_blocks; j++) {
                        fwrite(zone_buff, sizeof(char), block_size, args->dest);
                    }

                    bytes_left -= read_bytes;

                    continue;   // Skip to next indirect
                }

                // Process direct zones of indirect zones
                intptr_t indirect_addr = partition_addr + 
                                        (double_zones[i] * zone_size);
                fseek(args->image, indirect_addr, SEEK_SET);
                fread(indirect_zones, sizeof(uint32_t), 
                        INDIRECT_ZONES, args->image);

                for (int k = 0;  k < INDIRECT_ZONES && bytes_left > 0; k++) {
                    uint32_t curr_zone = indirect_zones[k];
                    uint32_t num_bytes = block_size;
                    
                    // if number of bytes left is less than the size of zone
                    if (bytes_left < block_size) {
                        // number of bytes to read should be bytes left
                        num_bytes = bytes_left;
                    }

                    // check if deleted zone
                    if (curr_zone == 0) {
                        memset(zone_buff, 0, bytes_left);
                        fwrite(zone_buff, sizeof(char), bytes_left, 
                                        args->dest);
                        
                        // decrement number of bytes left to read
                        bytes_left -= num_bytes;
                        continue;
                    }

                    intptr_t zone_addr = partition_addr + 
                                            (curr_zone * zone_size);
                    fseek(args->image, zone_addr, SEEK_SET);
                    fread(zone_buff, sizeof(uint8_t), num_bytes, args->image);
                    fwrite(zone_buff, sizeof(char), num_bytes, args->dest);
                    bytes_left -= num_bytes;
                }
            }
        }
    }
}