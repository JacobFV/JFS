#ifndef VCB_HEADER
#define VCB_HEADER 1

#include "def.h"

struct volume_control_block {
    // TODO
};
typedef struct volume_control_block* VCB;

VCB new_VCB();
/*  Allocates and initializes a new VCB. */

error_t serialize_VCB(VCB vcb, int64_t* len, byte* data);
/*  Serializes a VCB to a byte array and sets length */

#endif