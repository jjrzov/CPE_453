#include "mincommon.h"

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
    while ((opt = getopt(argc, argv, "vp:s")) != -1) {
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
                break;  // Maybe missing print usage???
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
    return;
}

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

    return;
}

bool isValidFS(SuperBlock_t *block) {
    return (block->magic == MINIX_MAGIC_NUM);
}

bool isValidPartition(uint8_t *block) {
    return (block[510] == VALID_BYTE510) && (block[511] == VALID_BYTE511);
}

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