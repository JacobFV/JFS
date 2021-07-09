#include<stdint.h>
#include<stdexcept>
#include<stdlib.h>

struct raidinfo {
    int8_t mirrors;
    int8_t chains;
    int8_t stripes;
    int8_t num_disks;
    char* paths;
};
typedef struct raidinfo* RAIDINFO;