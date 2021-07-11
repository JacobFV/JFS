#ifndef FSUTILS_HEADER
#define FSUTILS_HEADER 1

#include "def.h"

USER user_from_uname(char* uname, VCB vcb);

char** separate_path_list(char* path_list);
/*  Separates single-string path lists

    E.G.: separates "/usr/bin//second/path//third/path"
    into: "/usr/bin", "/second/path", and "/third/path" */

bool authenticate(char* path, bool read, bool write, bool execute,
                  USER user, VCB vcb, RAID raid);
/*  Determines is user is allowed to perform a certain
    set of operations on an inode at `path`. */

void create_inode_and_jfile(char* path, FILE* fp, 
                            FILEATTR* extra_attributes,
                            USER user, VCB vcb, RAID raid);
/*  Policy:
    Generates dir or symlink at bloc_loc with appropriate attributes
    and manages vcb logic for free_space, free_blocks, num_files, num_dirs,
    num_symlinks, total_inodes, master_inode_table, and next_free_block. 

    Mechanism:
    1. create empty JFILE
    2. add extra_attributes to jfile
    3. add the following attributes and values if they
        were not encountered in extra_attributes
        - owner = user
        - file_size = 0 (will be set later)
        - disk_space = 0 (will be set later)
        - user_permissions = 7
        - everybody_permissions = 7
        - last_read_dt = get time
        - last_write_dt = get time
        - last_exec_dt = 0
        - serialized_byte_len = 0
        - serialized_bloc_len = 0
    4. link inodes
        1. add jfile to the children (`dests`) of its containing directory 
        2. add containing directory to the incoming links attribute of this file
    5. determine if the jfile represents a file, dir, or symlink
        A. files have a `file_size` attribute
            - add the file pointer to jfile
        B. dirs have a `dests` attribute. fp=NULL
        C. symlinks have a `links_to` attribute. fp=NULL
    6. use `save_jfile` to save jfile to jfs
    7. modify VCB parameters:
        1. increment num inodes
        2A. if is file: increment num_files 
        2B. if is dir: increment num_dirs 
        2C. if is symlink: increment num_symlinks */

JFILE open_jfile(char* path, bool fixcorrupt, 
                 USER user, VCB vcb, RAID raid);
/*  Policy:
    Opens JFILE from jfs disk and manages vcb logic

    Mechanism: 
    1. find block location of file
    2. deserialize file from data linked across blocks into a JFILE
    3. modify the JFILE last_read_dt attribute to now
    4. return the JFILE */

void save_jfile(char* path, JFILE jfile,
                USER user, VCB vcb, RAID raid);
/*  Policy:
    Serializes a JFILE onto Jacob's filesystem.

    Mechanism:
    1. serialize the jfile attributes
    2. use `write_raid_bytes_linked` to over-write the serialized jfile onto the jfs
        1. decrease free_blocks by new_blocks_written*
        2. decrease free_space by bytes_len of serialized bytes
    7. update serialized_byte_len and serialized_bloc_len of jfile
        to match the actual serialized length and blocks used. Then
        repeat steps 1 and 2 once.
    3. modify the JFILE last_write_dt attribute to now
    4. foreward return code from write_raid_bytes_linked */



# endif /* FSUTILS_HEADER */