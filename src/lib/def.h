#ifndef DEF_HEADER
#define DEF_HEADER 1

#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<stdbool.h>
#include<string.h>

typedef int8_t byte;
typedef int64_t BYTE_LOC;
typedef int64_t BLOC_LOC;
typedef int8_t* BLOCK;
typedef int8_t* BLOC_DATA;
const int32_t N_PARITY_BITS = 8;
const int32_t N_PARITY = N_PARITY_BITS / 8;

struct RAIDINFO_struct {
    int8_t mirrors;
    int8_t chains;
    int8_t stripes;
    int8_t num_disks;
    char* paths;
};
typedef struct RAIDINFO_struct* RAIDINFO;

struct RAID_struct {
    int8_t mirrors;
    int8_t chains;
    int8_t stripes;
    int8_t num_disks;
    FILE** files;
};
typedef struct RAID_struct* RAID;

typedef int8_t USER;

struct INODE_struct {
    char* path; // absolute path
    BLOC_LOC start_block;
};
typedef struct INODE_struct* INODE;

struct VCB_struct {
    int64_t block_size;
    int64_t jfs_format_version_major;
    int64_t jfs_format_version_minor;
    char* volume_name;
    int64_t datetime_last_formatted;
    BLOC_LOC total_space; // raw bytes
    BLOC_LOC max_possible_usable_space; // potentially usable bytes subtracting fs overhead
    BLOC_LOC free_space; // bytes
    BLOC_LOC free_blocks; // blocks
    BLOC_LOC num_files;
    BLOC_LOC num_dirs;
    BLOC_LOC num_symlinks;
    BLOC_LOC total_inodes;
    INODE* master_inode_table;
    BLOC_LOC next_free_block;
    int16_t num_reserved_chains;
    BLOC_LOC** reserved_chain_starting_blocs;
    int8_t num_users;
    char** unames;
};
typedef struct VCB_struct* VCB;

struct FILEATTR_struct {
    int8_t id;
    void* val;
};
typedef struct FILEATTR_struct* FILEATTR;

struct JFILE_struct {
    int8_t num_attrs;
    FILEATTR* attrs;
    byte* content;
};
typedef struct JFILE_struct JFILE;

#endif /* DEF_HEADER */