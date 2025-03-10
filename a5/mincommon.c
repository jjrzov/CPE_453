#include "mincommon.h"

void parseArgs(int argc, char *argv[], bool func, Args_t* args) {
    // Initialize args struct
    args->part_number = -1;
    args->subpart_number = -1;

    if ((func && MINLS_BOOL) && (argc < MIN_MINLS_ARGS)) {
        printUsage(MINLS_BOOL);
        exit(EXIT_FAILURE);
    } else if ((func && !MINLS_BOOL) && (argc < MIN_MINGET_ARGS)) {
        printUsage(!MINLS_BOOL);
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


}

void printUsage(bool func) {
    if (func && MINLS_BOOL) {
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