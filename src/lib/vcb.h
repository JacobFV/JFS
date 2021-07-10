#ifndef VCB_HEADER
#define VCB_HEADER 1

#include "def.h"


VCB new_VCB();
/*  Allocates and initializes a new VCB. */

void serialize_VCB(VCB vcb, int64_t** len, byte* data);
/*  Serializes a VCB to a byte array and sets length */

#endif /* VCB_HEADER */