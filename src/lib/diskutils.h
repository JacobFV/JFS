#ifndef DISKUTILS_HEADER
#define DISKUTILS_HEADER 1

#include "def.h"


ERR find_next_free_block(BLOC_LOC starting_block, int64_t bloc_size, 
                         BLOC_LOC* next_free, RAID raid);
/*  Finds next free block on raid (where valid bit = 0). 
    Goes from starting_block to end and then restarts search
    from after VCB definition until right before the start.
    
    Returns -1 and prints message if there are no free blocks.
    Forewards -1 but does not print if any errors are encountered by subroutines
    Otherwise silently returns 0 */

ERR write_raid_bytes_raw(BYTE_LOC byte_loc, BYTE_LOC bytes_len, byte* bytes, RAID raid);
/*  Writes raw bytes from `byte_loc` to `byte_loc+bytes_len` onto RAID
    using mirroring, chaining, and striping as indicated by raid.
    
    Does not perform any corruption checking.
    
    Returns -1 and prints message if there are any errors writing.
    Otherwise silently returns 0. */

ERR read_raid_bytes_raw(BYTE_LOC byte_loc, BYTE_LOC bytes_len, byte* bytes, RAID raid); 
/*  Writes raw bytes from `byte_loc` to `byte_loc+bytes_len` onto RAID
    using mirroring, chaining, and striping as indicated by raid.

    If there are mirror conflicts during reading,
    1. tries selecting the majority bit
    2. or takes the bit at mirror #1 if there is no majority
    
    Returns -1 and prints message if there are any errors reading
    Otherwise silently returns 0 */

ERR write_raid_block(BLOC_LOC bloc_loc, BLOC_DATA data, int64_t bloc_size, RAID raid);
/*  Writes `data` onto block located at `bloc_loc`.
    This involves three steps:
    1. set the valid bit,
    2. write the bytes in `data`,
    3. compute and assign the parity bits.
    
    Forewards -1 if any errors are encountered by subroutines
    Otherwise silently returns 0 */

ERR read_raid_block(BLOC_LOC bloc_loc, BLOC_DATA data, int64_t bloc_size, 
                    bool fixcorrupt, RAID raid); 
/*  Stores data bytes in block located at `loc_loc` onto `data`.
    (Reads `block_size*bloc_size-1-N_PARITY` bytes starting from `block_size*bloc_loc+1`)
    
    Does perform corruption detection and optionally correction

    Returns -1 and prints message if corruption is detected but fixcorrupt is false
    Forewards -1 but does not print if any errors are encountered by subroutines
    Otherwise silently returns 0 */

ERR write_raid_bytes_linked(BLOC_LOC starting_block, BYTE_LOC bytes_len, int8_t* bytes,
                            BLOC_LOC* new_blocks_written, BLOC_LOC* next_free_block, 
                            RAID raid);
/*  Writes `bytes` onto the blocks begining at `starting_block` 
    until `bytes_len` bytes have been writen to sequential blocks.

    If the starting block was already valid, this function follows the linked
    block chain until either all bytes have been written or the block being
    written to has no next link but more bytes need to be written. If the linked
    blocks run out, this function allocates, links, and writes to more blocks. 
    
    If the starting block was not already written, this function allocates, 
    links, and writes to more blocks.

    May also reassign the next_free_block if it had to allocate
    another block. The next_free_block will stall at the last block
    written when there are no more free blocks.

    Also assigns the `new_blocks_written` pointer.

    Forewards -1 but does not print if any errors are encountered by subroutines
    Otherwise silently returns 0 */

ERR read_raid_bytes_linked(BLOC_LOC starting_block, BYTE_LOC bytes_len, int8_t* bytes,
                           bool fixcorrupt, RAID raid);
/*  Reads the bytes along linked blocks begining at `starting_block` 
    until `bytes_len` bytes have been written to `bytes` or no more
    linked blocks are found.

    Does perform corruption detection and optionally correction

    Forewards -1 but does not print if any errors are encountered by subroutines
    Otherwise silently returns 0 */


#endif /* DISKUTILS_HEADER */