#ifndef DEF_HEADER
#define DEF_HEADER 1

#include<stdint.h>
#include<stdexcept>
#include<stdlib.h>
#include<stdbool.h>

typedef int8_t byte;
typedef int64_t BYTE_LOC;
typedef int64_t BLOC_LOC;
typedef int8_t* BLOCK;
typedef int8_t* BLOC_DATA;


struct raidinfo {
    int8_t mirrors;
    int8_t chains;
    int8_t stripes;
    int8_t num_disks;
    char* paths;
};
typedef struct raidinfo* RAIDINFO;

struct raid {
    int8_t mirrors;
    int8_t chains;
    int8_t stripes;
    int8_t num_disks;
    FILE** files;
};
typedef struct raid* RAID;


struct volume_control_block {
    // TODO
};
typedef struct volume_control_block* VCB;

#define MIRRORS_DIFFERENT 123001
#define INVALID_BLOCK_READ 123002
#define NO_INVALID_BLOCKS 123003


typedef int8_t USER;

#endif /* DEF_HEADER */