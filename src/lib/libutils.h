#ifndef LIBUTILS_HEADER
#define LIBUTILS_HEADER 1

#include "def.h"

void print_error(error_t jfs_error);
/*  Prints the user-readable form of any described by 
    a error_t jfs error code */

void free_RAIDINFO(RAIDINFO raidinfo);
/*  Frees raidinfo */

void free_RAID(RAID raid);
/*  Closes and frees raid */

void free_inode(INODE inode);
/*  Frees inode */

void free_VCB(VCB vcb, RAID raid);
/*  Saves and frees vcb. */

void free_FILEATTR(FILEATTR fileattr);
/*  Frees fileattr. */

void free_JFILE(JFILE jfile);
/*  Saves and frees jfile. */

#endif /* LIBUTILS_HEADER */