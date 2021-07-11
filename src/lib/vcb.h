#ifndef VCB_HEADER
#define VCB_HEADER 1

#include "def.h"


VCB new_VCB();
/*  Allocates and initializes a new VCB. */

ERR load_VCB(VCB* vcb, RAID raid);
/*  Loads VCB from raid 

    Forewards -1 if any errors are encountered by subroutines
    Otherwise silently returns 0 */

ERR save_VCB(VCB vcb, RAID raid);
/*  Saves vcb to raid

    Forewards -1 if any errors are encountered by subroutines
    Otherwise silently returns 0 */

#endif /* VCB_HEADER */