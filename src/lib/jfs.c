#include "bits.h"
#include "jfs.h"
#include "bits.h"
#include "libutils.h"
#include "diskutils.h"
#include "vcb.h"
#include "fsutils.h"


error_t create(int64_t num_blocks, int32_t block_size, 
               char* volume_name, RAIDINFO desired_raid_info);
/*  Policy: 
    Creates new disk(s) and then formats a new volume on those disks.
    Disks are equally sized and ceiling rounded if necesary. 

    Mechanism:
    1. create disk files
    2. use backend `format` to format new disk files
    3. use `print_error` to print and also return
        error codes encountered */
{
    
}