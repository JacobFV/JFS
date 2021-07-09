#include "header.h"

struct volume_control_block {
    // TODO
};
typedef struct volume_control_block* VCB;

struct raid {
    int8_t mirrors;
    int8_t chains;
    int8_t stripes;
    int8_t num_disks;
    FILE** files;
};
typedef struct raid* RAID;

typedef int8_t byte;
typedef int64_t BYTE_LOC;
typedef int64_t BLOC_LOC;
typedef int8_t* BLOCK;
typedef int8_t* BLOC_DATA;

#define MIRRORS_DIFFERENT 75432
#define INVALID_BLOCK_READ 27983
#define NO_INVALID_BLOCKS 64653
#define  132346


VCB new_VCB();
// allocates and initializes a new VCB

error_t serialize_VCB(VCB vcb, int64_t* len, byte* data);
// serializes a VCB to a byte array and sets length
// foreward any error codes (but probabbly has seg fault) if at all.

error_t write_raid_bytes(BYTE_LOC byte_loc, BYTE_LOC byte_len, int8_t* bytes, RAID raid);
// writes raw bytes from `loc` to `loc+len`
// propperly locates bytes according to `raid
// no logical error or corruption checking
// foreward any error codes.

error_t read_raid_bytes(BYTE_LOC byte_loc, BYTE_LOC byte_len, int8_t* bytes, RAID raid); 
/*read raw bytes from `loc` to `loc+len`
     if there are mirror conflicts:
         pick the majority or index #1 if symmetric
 if there was any mirror conflict, return MIRRORS_DIFFERENT
 otherwise, foreward any error codes */

error_t write_raid_block(BLOC_LOC bloc_loc, BLOC_DATA data, RAID raid, int64_t block_size);
// 1. writes the bytes in block at `bloc_loc`
// 2. writes the valid and parity bits in a single final byte
// foreward any error codes.

error_t read_raid_block(BLOC_LOC bloc_loc, BLOC_DATA data, RAID raid, int64_t block_size); 
// read block at `bloc_loc`
// if valid is false: return 
// if the parity is incorrect, it returns an error (-1) but still performs its job

error_t find_next_free_block(BLOC_LOC starting, int64_t block_size, BLOC_LOC* next_free);
// finds next free block on raid (valid bit = 0)