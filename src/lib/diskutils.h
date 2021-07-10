#ifndef DISKUTILS_HEADER
#define DISKUTILS_HEADER 1

#include "def.h"


error_t write_raid_bytes(BYTE_LOC byte_loc, BYTE_LOC byte_len, int8_t* bytes, RAID raid);
/*  Writes raw bytes from `loc` to `loc+len`
    located propperly bytes according to `raid`
    Does not perform any corruption checking
    Forewards the last error code from writing to file. */

error_t read_raid_bytes(BYTE_LOC byte_loc, BYTE_LOC byte_len, int8_t* bytes, RAID raid); 
/*  reads raw bytes from `loc` to `loc+len`
    If there are mirror conflicts during reading,
    selects the bit which most mirrors have or index #1 if symmetric
    Returns MIRRORS_DIFFERENT if there was any mirror conflict,
    otherwise, forewards the last error code from writing to file. */

error_t write_raid_block(BLOC_LOC bloc_loc, BLOC_DATA data, RAID raid, int64_t block_size);
/*  Sets the valid bit, writes the bytes in block at `bloc_loc`,
    and computes and assigns the parity bit. Forewards the return
    code from `write_raid_bytes`. */

error_t read_raid_block(BLOC_LOC bloc_loc, BLOC_DATA data, RAID raid, int64_t block_size); 
/*  Assigns `data` to equal the byte contents at `bloc_loc`.
    If the valid bit is false, returns a INVALID_BLOCK_READ error.
    Otherwise, forewards the return code from `read_raid_bytes`. */

error_t find_next_free_block(BLOC_LOC starting, int64_t block_size, BLOC_LOC* next_free);
/*  Finds next free block on raid (where valid bit = 0). */

#endif /* DISKUTILS_HEADER */