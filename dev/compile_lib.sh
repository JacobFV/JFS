#!/bin/sh
cat ./README.md
gcc -c jfs.i \
    src/lib/bits.c \
    src/lib/diskutils.c \
    src/lib/fsutils.c \
    src/lib/jfs.c \
    src/lib/libutils.c \
    src/lib/vcb.c