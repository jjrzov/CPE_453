#include "mincommon.h"

Inode_t *inodes;

/**
 * Traverse through the given image path to find its correlated inode number
 * within the image.
 */
uint32_t findInode(char *path, Args_t *args, size_t zone_size, 
                    intptr_t partition_addr, size_t block_size) {
    if (args->verbose) {
        printf("Searching for %s\n", path);
    }
    char path_copy[MAX_PATH_SIZE];
    strcpy(path_copy, path);
    char *path_token = strtok(path_copy, "/");

    bool found = true;
    Inode_t *curr_inode = inodes;
    uint32_t curr_inode_ind = 0;

    uint32_t indirect_zones[INDIRECT_ZONES];
    uint32_t double_zones[INDIRECT_ZONES];

    while (path_token) {
        int i;
        found = false;
        uint32_t bytes_left = curr_inode->size;
        if (args->verbose) {
            printf("Opening: %s\n", path_token);
            printf("   Current inode: %d\n", curr_inode_ind);
        }

        if (!(curr_inode->mode & DIRECTORY)) {
            perror("Error: Not a directory\n");
            exit(EXIT_FAILURE);
        }

        for (i = 0;  i < DIRECT_ZONES && bytes_left > 0; i++) {
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
            int ind = checkZone(args, zone_addr, zone_size, 
                                    path_token, num_bytes);
            bytes_left -= num_bytes;

            if (ind) {
                curr_inode_ind = ind - 1;
                curr_inode = inodes + curr_inode_ind;
                found = true;
            }
        }

        if (!found && bytes_left > 0) {
            if (curr_inode->indirect == 0) {
                // Hole detected => zones referred to are to be treated as zero
                uint32_t hole_bytes = (INDIRECT_ZONES * block_size);
                
                if (bytes_left < hole_bytes) {
                    bytes_left -= bytes_left;
                } else {
                    bytes_left -= hole_bytes;
                }

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

                    // check if deleted zone
                    if (curr_zone == 0) {
                        // decrement number of bytes left to read
                        bytes_left -= num_bytes;
                        continue;
                    }

                    // seek/read num_bytes at zone address
                    intptr_t zone_addr = partition_addr + 
                                            (curr_zone * zone_size);
                    int ind = checkZone(args, zone_addr, zone_size, 
                                            path_token, num_bytes);
                    bytes_left -= num_bytes;

                    if (ind) {
                        curr_inode_ind = ind - 1;
                        curr_inode = inodes + curr_inode_ind;
                        found = true;
                    }
                }
            }
        }

        if (!found && bytes_left > 0) {
            if (curr_inode->two_indirect == 0) {
                // Hole detected => zones referred to are to be treated as zero
                uint32_t hole_bytes = (INDIRECT_ZONES * INDIRECT_ZONES * 
                                            block_size);
                
                if (bytes_left < hole_bytes) {
                    bytes_left = 0; // If all holes then its done
                } else {
                    bytes_left -= hole_bytes;    
                }

            } else {
                intptr_t duo_indirect_addr = partition_addr + 
                                        (curr_inode->two_indirect * zone_size);
                fseek(args->image, duo_indirect_addr, SEEK_SET);
                fread(double_zones, sizeof(uint32_t), 
                INDIRECT_ZONES, args->image);

                for (i = 0;  i < INDIRECT_ZONES && bytes_left > 0; i++) {
                    uint32_t curr_indirect_zone = double_zones[i];

                    if (curr_indirect_zone == 0) {
                        // Hole within the indirect zone
                        uint32_t hole_bytes = (INDIRECT_ZONES * block_size);
                
                        if (bytes_left < hole_bytes) {
                            bytes_left -= bytes_left;
                        } else {
                            bytes_left -= hole_bytes;
                        }
                        continue;   // Skip to next indirect
                    }

                    // Process direct zones of indirect zones
                    intptr_t indirect_addr = partition_addr + 
                                            (curr_inode->indirect * zone_size);
                    fseek(args->image, indirect_addr, SEEK_SET);
                    fread(indirect_zones, sizeof(uint32_t), 
                            INDIRECT_ZONES, args->image);

                    for (int j = 0;  j < INDIRECT_ZONES && bytes_left > 0; j++){
                        uint32_t curr_zone = indirect_zones[j];
                        uint32_t num_bytes = block_size;
                        
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
                        int ind = checkZone(args, zone_addr, zone_size, 
                                                path_token, num_bytes);
                        bytes_left -= num_bytes;

                        if (ind) {
                            curr_inode_ind = ind - 1;
                            curr_inode = inodes + curr_inode_ind;
                            found = true;
                        }
                    }
                }
            }
        }

        // increment path_token
        path_token = strtok(NULL, "/");
    }

    if (found) {
        return curr_inode_ind + 1;
    } else {
        return 0;
    }
}

/**
 * Given a zone and name, finds the corresponding directory entry and returns
 * its inode index
 */
uint32_t checkZone(Args_t *args, intptr_t zone_addr, size_t zone_size, 
                char *path_token, uint32_t num_bytes) {
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
        } else if (strcmp(curr_dir->name, path_token) == 0) {
            // if name of directory entry matches path_token
            if(args->verbose) {
                printf("Found inode: (%s, %d)\n", 
                    curr_dir->name, curr_dir->inode);
            }

            // update current inode to found directory
            return curr_dir->inode;
        }
    }

    return 0;
}

/**
 * Parse through input arguments for a minls or minget command and store into 
 * Args_t struct 
 */
void parseArgs(int argc, char *argv[], bool func, Args_t *args) {
    // Initialize args struct
    args->part_number = -1;
    args->subpart_number = -1;

    if (func && (argc < MIN_MINLS_ARGS)) {
        // if minls and valid number of arguments
        printUsage(func);
        exit(EXIT_FAILURE);
    } else if (!func && (argc < MIN_MINGET_ARGS)) {
        // if minget and valid number of arguments
        printUsage(func);
        exit(EXIT_FAILURE);
    }

    int opt;
    // Go through the flags
    while ((opt = getopt(argc, argv, "vp:s:")) != -1) {
        switch (opt) {
            case 'v':
                args->verbose = true;
                break;
            case 'p':
                if (strlen(optarg) == 0) {
                    perror("Error: No Part Number\n");
                    exit(EXIT_FAILURE);
                }
                args->has_part = true;
                args->part_number = atoi(optarg);
                if (args->part_number < 0 || args->part_number > 3) {
                    perror("Error: Invalid Part Number\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 's':
                if (args->has_part) {
                    if (strlen(optarg) == 0) {
                        perror("Error: No Subpart Number\n");
                        exit(EXIT_FAILURE);
                    }

                    args->subpart_number = atoi(optarg);
                    if (args->subpart_number < 0 || args->subpart_number > 3) {
                        perror("Error: Invalid Subpart Number\n");
                        exit(EXIT_FAILURE);
                    }
                } else {
                    perror("Error: Cannot have subpart and no part number\n");
                    exit(EXIT_FAILURE);
                }

                break;
            default:
                break;
        }
    }

    // Optind now points to image
    if (optind < argc) {
        if ((args->image = fopen(argv[optind], "r")) == NULL) {
            perror("Error: Could not open image\n");
            exit(EXIT_FAILURE);  
        }
    }

    optind++;   // Increment to next arg
    if (func) {
        // MINLS => get path
        if (optind < argc) {
            strncpy(args->image_path, argv[optind], MAX_PATH_SIZE);
        } else {
            // Set default as path
            strncpy(args->image_path, DEFAULT_PATH, MAX_PATH_SIZE);
        }
    } else {
        // MINGET => get src and dest path
        if (optind < argc) {
            strncpy(args->src_path, argv[optind], MAX_PATH_SIZE);
            
            optind++;
            if (optind < argc) {
                if ((args->dest = fopen(argv[optind], "w")) == NULL) {
                    perror("Error: Could not open destination\n");
                    exit(EXIT_FAILURE);  
                } 
            } else {
                // Set to default (stdout)
                args->dest = stdout;    // stdout already a file pointer
            }
        }
    }
    return;
}
/**
 * Fills out the partition table for a primary or sub partition from the 
 * boot sector
 */
void parsePartitionTable(Args_t *args, PartitionTableEntry_t *part_table) {
    if (!args->has_part) {
        return; // Unpartitioned
    }

    // Get Boot Sector which has partition table
    uint8_t block[BOOT_BLOCK_SIZE];

    fseek(args->image, 0, SEEK_SET);    // Set to beginning of file
    fread(block, sizeof(uint8_t), BOOT_BLOCK_SIZE, args->image);

    // Check for partition signature
    if (!isValidPartition(block)) {
        perror("Error: Not a valid partition\n");
        exit(EXIT_FAILURE);
    }

    /**
     * Go to offset for partition table (0x1BE), then skip over partition
     * tables until partition number being requested 
     */ 
    PartitionTableEntry_t *temp_table;
    temp_table = ((PartitionTableEntry_t *) (block + PART_TBL_OFFSET)) 
                        + args->part_number;

    if (temp_table->type != MINIX_PART_TYPE) {
        perror("Error: Invalid Partition Type\n");
        exit(EXIT_FAILURE);
    }

    if (args->subpart_number < 0) {
        // No subpartitions
        memcpy(part_table, temp_table, sizeof(PartitionTableEntry_t));
        if (args->verbose) {
            printPartitionTable(part_table, PRI);
        }
        return;
    }

    /**
     * Currently have the requested primary partition's table, need to get to 
     * the boot sector (first sector) of the primary partition (has 
     * subpartitions)  
     */
    fseek(args->image, temp_table->lFirst * SECTOR_SIZE, SEEK_SET);
    fread(block, sizeof(uint8_t), BOOT_BLOCK_SIZE, args->image);

    // Perform same operations as primary partition
    if (!isValidPartition(block)) {
        perror("Error: Not a valid partition\n");
        exit(EXIT_FAILURE);
    }

    temp_table = ((PartitionTableEntry_t *) (block + PART_TBL_OFFSET)) 
                        + args->subpart_number;

    if (temp_table->type != MINIX_PART_TYPE) {
        perror("Error: Invalid Partition Type\n");
        exit(EXIT_FAILURE);
    }

    memcpy(part_table, temp_table, sizeof(PartitionTableEntry_t));
    if (args->verbose) {
        printPartitionTable(part_table, !PRI);
    }
    return;
}

/**
 * Parses the image for the superblock and stores the superblock info into
 * a struct SuperBlock_t
 */
void parseSuperBlock(Args_t *args, PartitionTableEntry_t *part_table,
                        SuperBlock_t *super_blk) {

    size_t super_blk_addr;
    if (args->has_part) {
        super_blk_addr = (part_table->lFirst * SECTOR_SIZE) + SB_OFFSET;
    } else {
        super_blk_addr = SB_OFFSET; // Not partitioned
    }

    fseek(args->image, super_blk_addr, SEEK_SET);
    fread(super_blk, sizeof(SuperBlock_t), 1, args->image);
    
    if (!isValidFS(super_blk)) {
        perror("Error: Invalid filesystem\n");
        exit(EXIT_FAILURE);
    }
    if (args->verbose) {
        printSuperBlock(super_blk);
    }

    return;
}

/**
 * Prints Partition table for verbose flag
 */
void printPartitionTable(PartitionTableEntry_t *pt, bool sub) {
    if (!sub) {
        printf("Partition Table:\n");
    } else {
        printf("SubPartition Table:\n");
    }

    printf("   bootind: %d\n", pt->bootind);
    printf("   start_head: %d\n", pt->start_head);
    printf("   start_sec: %d\n", pt->start_sec);
    printf("   start_cyl: %d\n", pt->start_cyl);
    printf("   type: %d\n", pt->type);
    printf("   end_head: %d\n", pt->end_head);
    printf("   end_sec: %d\n", pt->end_sec);
    printf("   end_cyl: %d\n", pt->end_cyl);
    printf("   lFirst: %d\n", pt->lFirst);
    printf("   size: %d\n\n", pt->size);
}

/**
 * Prints Super Block for verbose flag
 */
void printSuperBlock(SuperBlock_t *sb) {
    printf("Super Block:\n   On Disk:\n");
    printf("      ninodes: %d\n", sb->ninodes);
    printf("      pad1: %d\n", sb->pad1);
    printf("      i_blocks: %d\n", sb->i_blocks);
    printf("      z_blocks: %d\n", sb->z_blocks);
    printf("      firstdata: %d\n", sb->firstdata);
    printf("      log_zone_size: %d\n", sb->log_zone_size);
    printf("      max_file: %u\n", sb->max_file);
    printf("      zones: %d\n", sb->zones);
    printf("      magic: %d\n", sb->magic);
    printf("      blocksize: %d\n", sb->blocksize);
    printf("      subversion: %d\n\n", sb->subversion);
}

/**
 * Checks if the FS the superblock is within is a valid FS
 */
bool isValidFS(SuperBlock_t *block) {
    return (block->magic == MINIX_MAGIC_NUM);
}

/**
 * Checks a partition tables signatures to check that it is valid
 */
bool isValidPartition(uint8_t *block) {
    return (block[510] == VALID_BYTE510) && (block[511] == VALID_BYTE511);
}

/**
 * Prints the usage of minls or minget for verbose flag
 */
void printUsage(bool func) {
    if (func) {
        fprintf(stderr,
            "usage: minls [ -v ] [ -p num [ -s num ] ] imagefile [ path ]\n"
            "Options:\n"
            "-p part     --- select partition for filesystem \
            (default: none)\n"
            "-s sub      --- select subpartition for filesystem \
            (default: none)\n"
            "-h help     --- print usage information and exit\n"
            "-v verbose  --- increase verbosity level\n");
    } else {
        fprintf(stderr,
            "usage: minget [ -v ] [ -p part [ -s subpart ] ] imagefile \
            srcpath [ dstpath ]\n"
            "Options:\n"
            "-p part     --- select partition for filesystem \
            (default: none)\n"
            "-s sub      --- select subpartition for filesystem \
            (default: none)\n"
            "-h help     --- print usage information and exit\n"
            "-v verbose  --- increase verbosity level\n");
    }
}