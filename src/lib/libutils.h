#ifndef LIBUTILS_HEADER
#define LIBUTILS_HEADER 1

#include "def.h"

FILEATTR new_fileattr(int8_t id, void* val);
/*  Makes a copy of args so you can free the ones you passed in on your own */

RAIDINFO new_raidinfo(int8_t mirrors, int8_t chains, 
                      int8_t stripes, int8_t num_disks,
                      char* paths);
/*  Makes new raidinfo object */

void free_RAIDINFO(RAIDINFO raidinfo);
/*  Frees raidinfo */

ERR open_RAID(RAIDINFO raidinfo, RAID* raid);
/*  Makes RAID pointer

    Returns -1 and prints message if there are errors opening the files for raid
    Forewards -1 but does not print if any errors are encountered by subroutines
    Otherwise silently returns 0 */

ERR free_RAID(RAID raid);
/*  Closes and frees raid 

    Returns -1 and prints message if any file errors are encountered
    Otherwise silently returns 0 */

void free_inode(INODE inode);
/*  Frees inode */

ERR free_VCB(VCB vcb, RAID raid);
/*  Saves and frees vcb.

    Forewards -1 but does not print if any errors are encountered by subroutines
    Otherwise silently returns 0 */

void free_FILEATTR(FILEATTR fileattr);
/*  Frees fileattr. */

ERR free_JFILE(JFILE jfile, VCB vcb, RAID raid);
/*  Saves and frees jfile.

    Forewards -1 but does not print if any errors are encountered by subroutines
    Otherwise silently returns 0 */

#endif /* LIBUTILS_HEADER */