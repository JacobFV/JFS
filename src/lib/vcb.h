#ifndef VCB_HEADER
#define VCB_HEADER 1

#include "def.h"


VCB new_VCB();
/*  Allocates and initializes a new VCB. */

VCB load_VCB(RAID raid);
/*  Loads VCB from raid */

void save_VCB(VCB vcb, RAID raid);
/*  Saves vcb to raid */

#endif /* VCB_HEADER */