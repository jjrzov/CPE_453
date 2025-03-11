#ifndef MINCOMMON_H
#define MINCOMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#define PART_TBL_OFFSET (0x1BE)
#define MINIX_PART_TYPE (0x81)
#define VALID_BYTE510   (0x55)
#define VALID_BYTE511   (0xAA)
#define MINIX_MAGIC_NUM (0x4D5A)
#define INODE_SIZE      (64)
#define DIR_ENTRY_SIZE  (64)
#define SECTOR_SIZE     (512)

#define SB_OFFSET       (1024)
#define BOOT_BLOCK_SIZE (512)
#define DEFAULT_PATH    ("/")

#define INVALID_INODE   (0)
#define ROOT_INODE      (1)

// #define SYM_LINK        (0xA000)
#define FILE_TYPE_MASK  (0xF000)
#define REGULAR_FILE    (0x8000)
#define DIRECTORY       (0x4000)
#define OWNER_READ      (0x100)
#define OWNER_WRITE     (0x80)
#define OWNER_EXEC      (0x40)
#define GROUP_READ      (0x20)
#define GROUP_WRITE     (0x10)
#define GROUP_EXEC      (8)
#define OTHER_READ      (4)
#define OTHER_WRITE     (2)
#define OTHER_EXEC      (1)

#define DIRECT_ZONES    (7)
#define INDIRECT_ZONES  (1024)

#define MAX_NAME_SIZE   (60)
#define MAX_PATH_SIZE   (4096)

#define MINLS_BOOL      (1)
#define MIN_MINLS_ARGS  (2)
#define MIN_MINGET_ARGS (3)

typedef struct __attribute__ ((__packed__)) PartitionTableEntry_t {
    uint8_t bootind;        //  Boot magic number (0x80 if bootable)
    uint8_t start_head;     //   Start of partition in CHS
    uint8_t start_sec;     
    uint8_t start_cyl;     
    uint8_t type;           //  Type of partition (0x81 is MINIX)
    uint8_t end_head;       //  head End of partition in CHS
    uint8_t end_sec;     
    uint8_t end_cyl;     
    uint32_t lFirst;        //  First sector (LBA addressing)
    uint32_t size;          //  size of partition (in sectors)
} PartitionTableEntry_t;

typedef struct __attribute__ ((__packed__)) SuperBlock_t { 
    /* Minix Version 3 Superblock
    * this structure found in fs/super.h
    * in minix 3.1.1
    */
    /* on disk. These fields and orientation are non–negotiable */
    uint32_t ninodes;       /* number of inodes in this filesystem */
    uint16_t pad1;          /* make things line up properly */
    int16_t i_blocks;       /* # of blocks used by inode bit map */
    int16_t z_blocks;       /* # of blocks used by zone bit map */
    uint16_t firstdata;     /* number of first data zone */
    int16_t log_zone_size;  /* log2 of blocks per zone */
    int16_t pad2;           /* make things line up again */
    uint32_t max_file;      /* maximum file size */
    uint32_t zones;         /* number of zones on disk */
    int16_t magic;          /* magic number */
    int16_t pad3;           /* make things line up again */
    uint16_t blocksize;     /* block size in bytes */
    uint8_t subversion;     /* filesystem sub–version */
} SuperBlock_t;

typedef struct __attribute__ ((__packed__)) Inode_t {
    uint16_t mode; /* mode */
    uint16_t links; /* number or links */
    uint16_t uid;
    uint16_t gid;
    uint32_t size;
    int32_t atime;
    int32_t mtime;
    int32_t ctime;
    uint32_t zone[DIRECT_ZONES];
    uint32_t indirect;
    uint32_t two_indirect;
    uint32_t unused;
} Inode_t;

typedef struct __attribute__ ((__packed__)) DirEntry_t {
    uint32_t inode;             // Inode number
    char name[MAX_NAME_SIZE];   // filename
} DirEntry_t;

typedef struct __attribute__ ((__packed__)) Args_t {
    bool verbose;
    bool has_part;
    int part_number;
    int subpart_number;
    FILE *image;
    char image_path[MAX_PATH_SIZE];
    char src_path[MAX_PATH_SIZE];
    FILE *dest;
} Args_t;


void parseArgs(int argc, char *argv[], bool func, Args_t* args);
void parsePartitionTable(Args_t *args, PartitionTableEntry_t *part_table);
void parseSuperBlock(Args_t *args, PartitionTableEntry_t *part_table,
    SuperBlock_t *super_blk);
bool isValidFS(SuperBlock_t *block);
bool isValidPartition(uint8_t *block);
void printUsage(bool func);
Inode_t findInode(Args_t *args, size_t zone_size, intptr_t partition_addr)

extern Inode_t *inodes;

#endif // MINCOMMON_H