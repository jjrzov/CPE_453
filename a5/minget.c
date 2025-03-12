#include "mincommon.h"

void printInodeFileContents(uint32_t ind, Args_t *args, size_t zone_size, 
                    intptr_t partition_addr, size_t block_size);

Inode_t *inodes;

int main(int argc, char *argv[]) {
    Args_t args;
    PartitionTableEntry_t part_table;
    SuperBlock_t super_blk;

    parseArgs(argc, argv, !MINLS_BOOL, &args);
    // printf("Path?: %s, %s\n", args.image_path, args.src_path);
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
    uint32_t found_inode_ind = findInode(args.src_path, &args, zone_size, 
                                            part_addr, super_blk.blocksize);
    // printf("Inode: %d\n", found_inode_ind);
    if (!found_inode_ind) {
        perror("Error: File not found\n");
        exit(EXIT_FAILURE);
    } 

    // printf("Args: %s, %s\n", args.src_path, args.dest);

    Inode_t *found_inode = inodes + found_inode_ind - 1;

    if ((found_inode->mode & FILE_TYPE_MASK) != REGULAR_FILE) {
        perror("Error: Not a file\n");
        exit(EXIT_FAILURE);
    }

    printInodeFileContents(found_inode_ind, &args, zone_size, part_addr, 
                    super_blk.blocksize);
}


void printInodeFileContents(uint32_t ind, Args_t *args, size_t zone_size, 
                    intptr_t partition_addr, size_t block_size) {
    // printf("Entered findInode with path: %s\n", args->image_path);

    uint32_t curr_inode_ind = ind - 1;
    Inode_t *curr_inode = inodes + curr_inode_ind;

    uint8_t zone_buff[zone_size];
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

    //TODO: double indirect zones
}