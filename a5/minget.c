#include "mincommon.h"

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

    if (!found_inode_ind) {
        perror("Error: File not found\n");
        exit(EXIT_FAILURE);
    }

    Inode_t *found_inode = inodes + found_inode_ind - 1;
}