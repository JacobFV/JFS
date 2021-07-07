# jFS

Jacob's file system (jFS) is a FAT-inspired file system that can be invoked directly from the operating system. It currently requires an OS-provided filesystem substrate, but in the future jOS will directly utilize jFS to store data on drives.

Dr. Levine, this filesystem has all the bells and whistles for assignment 3: permissions, subdirectories, and go language implementation. 

``bash
jfs example ... # TODO
``

## Getting Started

### C
TODO

### Go
TODO

## Functionality

File/directory permissions inherit the permissions of their containing directory by default. The root drive gives `rwxrwx` to all its inodes. 

## Under the hood

Directories as file name tables. Inodes can have extended attributes (such as permissions) that are specified in the volume control block. These attributes are inherited from their containing directory/volume if not specified, and the disk space used for declaring them is dynamically assigned.

jFS utilizes 3 distinct data structures:
 - Volume control block (VCB)
    - version: 8
    - num users: 8
    - user names: N * 8
    - fnt length: 64
    - dabt length: 64
    - extended attribute table (EAT) len: 16
    - EAT items: N * MAX_EXTENDED_ATTR_LEN
 - File name table (FNT)
    - entries: N *
       - file name: MAX_FNAME_LEN
       - inode pointer: 64
 - Directory and attribute block pointer table (DABT)
    - entries: N *
       - file size: 16
       - last modified: 64
       - owner: 8
       - pointers to data blocks: M * 
  - Block pointer table (BPT)
    - entries: N *
       - 


Constants and limits (strings use 8-bit chars):
I will regret only pulling out a few constants and not more. 
 - `VERSION = 1`
 - `BLOCK_SIZE = 256` bytes
 - `MAX_FNAME_LEN = 56`
 - `MAX_PATH_LEN = 256`
 - `MAX_EXTENDED_ATTR_LEN = 256`
 - `MAX_BLOCKS = 2^40`