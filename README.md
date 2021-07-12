# jFS

🛠️ NOTE: Only the core sections of the header library have been implemented. I planned too big for this project.

Jacob's file system (jFS) is a file system library and CLI suite. It currently requires an OS-provided filesystem substrate, but a future operating system, jOS, will directly utilize jFS to store data on high throughput jDrives. This OS will be used to run jPhones, jPads, jMacs, and (of course) JavaScript.\*

Please see below for installation instructions.

## Examples

```bash
jfs create \
    NUM_BLOCKS=50000 \
    BLOCK_SIZE=256 \
    VOLUME_NAME=my_first_disk \
    DISKS=disk0,disk1,disk2,disk3,disk4,disk5,disk6,disk7,disk8,disk9,diska,diskb,diskc,diskd,diske,diskf \
    MIRRORS=1 \
    CHAINS=4 \
    STRIPED=true

jfs combine \
    NEWNAME=single_disk.jdf \
    DISKS=disk0,disk1,disk2,disk3,disk4,disk5,disk6,disk7,disk8,disk9,diska,diskb,diskc,diskd,diske,diskf \
    MIRRORS=1 \
    CHAINS=4 \
    STRIPED=true

jfs mount \
    DISKS=disk0,disk1,disk2,disk3,disk4,disk5,disk6,disk7,disk8,disk9,diska,diskb,diskc,diskd,diske,diskf \
    MIRRORS=1 \
    CHAINS=4 \
    STRIPED=true

jfs new_user student1

jfs mkdir home USER=nobody

jfs mkdir home/student1 USER=student1
```

```bash

# psuedo-dll usage example
$ ls
$ cat program.c
#include<unistd.h>
char** args[3] = {"get", "disk=mydisk.jdf", 
                  "internal_filepath=/path/to/my/file.txt",
                  "external_filepath=./file.txt"};
execve("jfs", args, NULL);
$ gcc program.c; ./program.c # run program.c
$ ls
file.txt
```

## Details

### Library

The jfs library provides core functionality for the CLI ir whatever awsome application you have in mind. Although core library functions explicitly return `error_t` codes, they may mutate arguments. (and some utility functions actually do return structs). Programmers linking jfs will want to include `lib/jfs.c` and may also want to make language-specific translations of the definitions in `lib/def.h`.

```c
// lib/jfs.h
# ifndef JFS_HEADER
# define JFS_HEADER 1

#include "def.h"


ERR create(int64_t num_blocks, int32_t block_size, 
           char* volume_name, RAIDINFO desired_raid_info);
/*  Policy: 
    Creates new disk(s) and then formats a new volume on those disks.
    Disks are equally sized and ceiling rounded if necesary. 

    Mechanism:
    1. create disk files
    2. use backend `format` to format new disk files 

    Returns -1 and prints message if 
        the desired_raid_info data is not correct
        error creating files
    Forewards -1 but does not print if any errors are encountered by subroutines
    Otherwise silently returns 0 */

ERR format(int64_t num_blocks, int32_t block_size, char* volume_name, RAID raid);
/*  Policy: 
    Formats disk(s). 

    Mechanism:
    1. use `write_raid_bytes_raw` to propperly write 0's 
        from 0 to `num_blocks*block_size`
    2. write volume control block
        1. generate VCB in memory with `new_VCB`
        2. serialize VCB to bytes with `save_VCB`

    Forewards -1 but does not print if any errors are encountered by subroutines
    Otherwise silently returns 0 */

ERR combine(char* new_name, RAID raid);
/*  Policy: 
    Combine all the disk(s) to a single disk file named NEWNAME. 

    1. use backend `create` to make a new single disk named `NEWNAME`
    2. copy all bits raw onto to the new disk.
        1. read and write blocks sequentially

    Returns -1 and prints error if newly created disk does not exactly match
        byte capacity of old raid
    Forewards -1 but does not print if any errors are encountered by subroutines
    Otherwise silently returns 0 */

ERR list(char* path, bool include_meta, 
         bool include_contents_hex, bool include_contents_char,
         bool recursive, VCB vcb, RAID raid, USER user);
/*  Policy: 
    List paths (and optionally meta-information and file contents)
    starting from `path`. Optionally lists recursively and across
    symlinks.

    Mechanism: 
    1. use `authenticate` to ensure the user has permission to read the inode at `path`
    2. print the absolute path of inode
    3. if `include_meta` is true: 
        1. print the attributes of the inode
    4. if `include_contents` is true and the inode has a filesize attribute: 
        1. print the contents of the inode
    5. if the inode at start_path is a directory and recursive is true: 
        1. use `list` on all inodes in the directory.

    Forewards -1 but does not print if any errors are encountered by subroutines
    Otherwise silently returns 0 */

ERR remove(char* path, JFILE jfile,
           VCB vcb, RAID raid, USER user);
/*  Policy: 
    Frees the space used by a single inode at PATH. Does not actually
    unlink the inode (unlinking is usually called after removing).

    Mechanism:
    1. use `authenticate` to ensure the user has permission to write the inode at `path`
    2. edit volume information
        - increase vcb free_space by serialized length of jfile
        - increase vcb free_blocks by block length of jfile
    3. assign all valid bits for the blocks used by the inode at PATH to false
    4. set vcb next_free_block to be the minimum of this inode's starting block
        and the previous vcb next_free_block

    Forewards -1 but does not print if any errors are encountered by subroutines
    Otherwise silently returns 0 */

ERR delete_inode(char* path, bool recursive, 
                 VCB vcb, RAID raid, USER user);
/*  Policy:
    Removes and unlinks an inode starting from PATH. Optionally 
    deletes recursively and across symlinks.

    Mechanism:
    1. use `authenticate` to ensure sure the user has permission 
        - to write to the inode at path
        - to write to the containing directory
    2. if recursive is true:
        A. if the inode was a directory:
            1. use `delete_inode` on all its children (recursively)
        B. if the inode was a symlink:
            1. use `delete_inode` on all its linked inode (recursively)
    3. use `remove` to remove the inode at `path` (not recursively)
    4. use `unlink` to unlink the inode at `path` (not recursively)

    Forewards -1 but does not print if any errors are encountered by subroutines
    Otherwise silently returns 0 */

ERR rename(char* oldpath, char* newpath, 
           bool fixsymlinks, bool recursive,
           VCB vcb, RAID raid, USER user);
/*  Policy:
    Renames / moves an inode from OLDPATH to NEWPATH. Optionally fixes
    symbolic links to newly moved inode. Optionally moves recursively
    (which allows entire directories to be moved) but does not move the
    symlinked files.

    Mechanism:
    1.  use `authenticate` to ensure the user has permission
        - to write to the recursive children starting at OLDPATH
        - to write to the containing directory of NEWPATH
    2. for each inode with have a matching prefix pattern as oldpath in the master inode table:
        1. change path prefix to newpath
        2. if the inode is a directory:
            1. rename the paths in its dests field.
        3. if fixsymlinks is true: 
            1. rename the paths of all symlinks into this inode 

    Forewards -1 but does not print if any errors are encountered by subroutines
    Otherwise silently returns 0*/

ERR put(char* external_filepath, char* internal_filepath, 
        VCB vcb, RAID raid, USER user);
/*  Policy:
    Puts (stores) a single file from host OS into the jfs disk.
    Does not delete external file from host OS filesystem. 

    Mechanism:
    1. use `authenticate` to ensure the user has permission to write 
        to the directory containing internal_filepath
    2. get file at external_path from OS
    3. use `create_inode_and_jfile` to add file to jfs

    Returns -1 and prints message if there are any errors opening external file.
    Forewards -1 but does not print if any errors are encountered by subroutines
    Otherwise silently returns 0 */

ERR get(char* internal_filepath, char* external_filepath, 
        VCB vcb, RAID raid, USER user);
/*  Policy:
    Gets (retrieves) a single file from Jacob's file system
    to put on the host OS filesystem. Does not delete internal
    file from Jacob's filesystem. 

    Mechanism:
    1. use `authenticate` to ensure the user has permission to
        read the inode at internal_filepath
    2. use `open_jfile` to open the file at internal_filepath
    3. create and open a new file in the host OS filesystem at 
        external_filepath for writing
    4. copy all bytes from the internal file to the external file. 
    
    Returns -1 and prints message if there are any errors opening external file.
    Forewards -1 but does not print if any errors are encountered by subroutines
    Otherwise silently returns 0 */

ERR set_user(char* path, USER new_user, bool recursive, 
             VCB vcb, RAID raid, USER user);
/*  Policy:
    Assigns the file at PATH to NEW_USER. Optionally changes ownership 
    recursively but not across symlinks (which allows entire directories
    to change ownership).

    Mechanism:
    1. use `open_jfile` to open jfile at path
    2. change the `owner` attribute to user
    4. if recursive and jfile is directory:
        1. run `set_user` on all children in directory
    5. use `save_jfile` to save jfile to jfs 

    Forewards -1 but does not print if any errors are encountered by subroutines
    Otherwise silently returns 0 */

ERR link(char* existing_path, char* new_path, 
         VCB vcb, RAID raid, USER user);
/*  Policy:
    Creates a link with path new_path to an inode at existing_path.

    Mechanism:
    1. use `authenticate` to ensure the user has permission 
        to write to the directory containing new_path
    2. use `create_inode_and_jfile` to make a jfile at new_path
        with attribute "links_to"=existing_path 
    3. add this link to the incoming links at new_path
        1. open jfile
        2. append path to symlink
        3. save jfile 

    Forewards -1 but does not print if any errors are encountered by subroutines
    Otherwise silently returns 0 */

ERR unlink(char* path, bool recursive,
           VCB vcb, RAID raid, USER user);
/*  Policy:
    Unlinks an inode at path. Optionally recursively across
    symlinks and down directories.

    Mechanism:
    1.  use `authenticate` to ensure the user has permission 
        - to write to the inode at path
        - to write to the containing directory
        - to write to all symlinks pointing to this inode
            (I just went ahead and deleted the symlinks here)
    2A. if inode is a file: 
        1. decrement vcb num_files
    2B. if the inode is a symlink: 
        1. decrement vcb num_symlinks
    2C. if inode is a directory: 
        1. decrement vcb num_dirs
        2. if recursive is true:
            1. use `unlink` on all files in the directory
    3. remove inode from master inode table
    4. decrement vcb total inodes
    5. remove inode from dests of containing dir

    Forewards -1 but does not print if any errors are encountered by subroutines
    Otherwise silently returns 0 */

ERR set_permissions(int8_t user_permissions, int8_t all_permissions, 
                    char* path, bool recursive, 
                    VCB vcb, RAID raid, USER user);
/*  Policy:
    Change permissions for an inode. USER_PERMISSIONS / ALL_PERMISSIONS
    are a single digit number 0-7 with standard UNIX bit flag meaning:
    (0=none, 1=only read, 2=only write, 3=read and write, 4=only exec,
    5=read and exec, 6=write and exec, and 7=read, write, and execute)
    Optionally sets permissions recursively, but not across symlinks.

    Mechanism:
    1. use `open_jfile` to open jfile at path
    2. change the `user_permissions` attribute to user_permissions
    3. change the `all_permissions` attribute to all_permissions
    4. if recursive and jfile is directory:
        1. run `set_permissions` on all children in directory
    5. use `save_jfile` to save jfile to jfs 

    Returns -1 and prints message if user/all_permissions is not in inclusive range 0..7
    Forewards -1 but does not print if any errors are encountered by subroutines
    Otherwise silently returns 0 */

ERR volume_info(VCB vcb);
/*  Policy: 
    Get data contained in volume control block.

    Mechanism: 
    1. for every field of VCB in the order declared
        1. print field value 
        
    Forewards -1 but does not print if any errors are encountered by subroutines
    Otherwise silently returns 0 */

ERR new_user(char* uname, VCB vcb);
/*  Policy:
    Adds a new user

    Mechanism:
    1. increment vcb num_users
    2. add user to vcb users 

    Forewards -1 but does not print if any errors are encountered by subroutines
    Otherwise silently returns 0 */

ERR remove_user(char* uname, VCB vcb);
/*  Policy:
    Removes a user

    Mechanism:
    1. look for and remove user from vcb users
    2. decrement vcb num_users

    Forewards -1 but does not print if any errors are encountered by subroutines
    Otherwise silently returns 0  */

ERR mkdir(char* path, VCB vcb, RAID raid, USER user);
/*  Policy:
    Makes a new directory at path.

    Mechanism:
    1. use `authenticate` to ensure the user has permission to write 
        to the directory containing path
    2. use `create_inode_and_jfile` to add an empty dir to jfs 

    Forewards -1 but does not print if any errors are encountered by subroutines
    Otherwise silently returns 0 */


# endif /* JFS_HEADER */
```

```c
// lib.fsutils.h

USER user_from_uname(char* uname, VCB vcb);

ATTRTYPE attrtype_from_id(int8_t id);
/*  Gets the (const) attribute type 
    for each (const) defined attribute.
    
    This code may be entirely compiler optimized. */

char* attr_name_from_id(int8_t id);
/*  Gets the (const) attribute name 
    for each (const) defined attribute.
    
    This code may be entirely compiler optimized. */

int attr_length_from_id(int8_t id);
/*  Gets the (const) attribute byte length 
    for each (const) defined attribute.
    
    This code may be entirely compiler optimized. */

int parent_slash_idx(char* path);
/*  Returns index of final slash in path
    (-1 if path already was root)

    E.G.: /path/file returns 5
          /long/path/file returns 10
          / returns 0 */

char** separate_path_list(char* path_list, int* num_separate);
/*  Separates single-string path lists

    E.G.: separates "/usr/bin//second/path//third/path"
    into: "/usr/bin", "/second/path", and "/third/path" */

char* combine_path_list(char** paths, int* num_separate);
/*  Combines and frees single-string path lists. 
    Reverse of `separate_path_list` */

bool authenticate(char* path, JFILE jfile,
                  bool read, bool write, bool execute,
                  USER user, VCB vcb);
/*  Determines is user is allowed to perform a certain
    set of operations on an inode at `path`.
    
    Prints message if user is not authenticated. */

ERR create_inode_and_jfile(char* path, FILE* fp, 
                           int8_t num_extra_attrs, FILEATTR* extra_attributes,
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
            - get size of file pointer
        B. dirs have a `dests` attribute. fp=NULL
        C. symlinks have a `links_to` attribute. fp=NULL
    6. use `save_jfile` to save jfile to jfs
    7. modify VCB parameters:
        1. increment num inodes
        2A. if is file: increment num_files 
        2B. if is dir: increment num_dirs 
        2C. if is symlink: increment num_symlinks
        3. create new inode from this file and add to vcb
        
    Forewards -1 if any errors are encountered by subroutines
    Otherwise silently returns */

ERR open_jfile(char* path, bool fixcorrupt, JFILE** jfile,
               VCB vcb, RAID raid);
/*  Policy:
    Opens JFILE from jfs disk and manages vcb logic

    Mechanism: 
    1. find block location of file
    2. deserialize file from data linked across blocks into a JFILE
    3. modify the JFILE last_read_dt attribute to now
    4. set jfile to be pointed to by the doubley indirect pointer
    
    Forewards -1 if any errors are encountered by subroutines
    Otherwise silently returns */

ERR save_jfile(char* path, JFILE jfile,
               USER user, VCB vcb, RAID raid);
/*  Policy:
    Serializes a JFILE onto Jacob's filesystem.

    Mechanism:
    1. serialize the jfile attributes
    TODO: take advantage of `serialize_file`
    2. use `write_raid_bytes_linked` to over-write the serialized jfile onto the jfs
        1. decrease free_blocks by new_blocks_written*
        2. decrease free_space by bytes_len of serialized bytes
    3. update serialized_byte_len and serialized_bloc_len of jfile
        to match the actual serialized length and blocks used. 
    4. repeat steps 1 and 2 once.
    5. modify the JFILE last_write_dt attribute to now
    6. foreward return code from write_raid_bytes_linked

    Forewards -1 if any errors are encountered by subroutines
    Otherwise silently returns 0 */

byte* serialize_file(char* path, JFILE jfile, int64_t* serialized_length, VCB vcb, RAID raid);
```

```c

ERR find_next_free_block(BLOC_LOC starting_block, int64_t bloc_size, 
                         BLOC_LOC* next_free, RAID raid);
/*  Finds next free block on raid (where valid bit = 0). 
    Goes from starting_block to end and then restarts search
    from after VCB definition until right before the start.
    
    Returns -1 and prints message if there are no free blocks.
    Forewards -1 but does not print if any errors are encountered by subroutines
    Otherwise silently returns 0 */

ERR write_raid_bytes_raw(BYTE_LOC byte_loc, BYTE_LOC bytes_len, byte* bytes, RAID raid);
/*  Writes raw bytes from `byte_loc` to `byte_loc+bytes_len` onto RAID
    using mirroring, chaining, and striping as indicated by raid.
    
    Does not perform any corruption checking.
    
    Returns -1 and prints message if there are any errors writing.
    Otherwise silently returns 0. */

ERR read_raid_bytes_raw(BYTE_LOC byte_loc, BYTE_LOC bytes_len, byte* bytes, RAID raid); 
/*  Writes raw bytes from `byte_loc` to `byte_loc+bytes_len` onto RAID
    using mirroring, chaining, and striping as indicated by raid.

    If there are mirror conflicts during reading,
    1. tries selecting the majority bit
    2. or takes the bit at mirror #1 if there is no majority
    
    Returns -1 and prints message if there are any errors reading
    Otherwise silently returns 0 */

ERR write_raid_block(BLOC_LOC bloc_loc, BLOC_DATA data, int64_t bloc_size, RAID raid);
/*  Writes `data` onto block located at `bloc_loc`.
    This involves three steps:
    1. set the valid bit,
    2. write the bytes in `data`,
    3. compute and assign the parity bits.
    
    Forewards -1 if any errors are encountered by subroutines
    Otherwise silently returns 0 */

ERR read_raid_block(BLOC_LOC bloc_loc, BLOC_DATA data, int64_t bloc_size, 
                    bool fixcorrupt, RAID raid); 
/*  Stores data bytes in block located at `loc_loc` onto `data`.
    (Reads `block_size*bloc_size-1-N_PARITY` bytes starting from `block_size*bloc_loc+1`)
    
    Does perform corruption detection and optionally correction

    Returns -1 and prints message if corruption is detected but fixcorrupt is false
    Forewards -1 but does not print if any errors are encountered by subroutines
    Otherwise silently returns 0 */

ERR write_raid_bytes_linked(BLOC_LOC starting_block, BYTE_LOC bytes_len, int8_t* bytes,
                            BLOC_LOC* new_blocks_written, BLOC_LOC* next_free_block, 
                            RAID raid);
/*  Writes `bytes` onto the blocks begining at `starting_block` 
    until `bytes_len` bytes have been writen to sequential blocks.

    If the starting block was already valid, this function follows the linked
    block chain until either all bytes have been written or the block being
    written to has no next link but more bytes need to be written. If the linked
    blocks run out, this function allocates, links, and writes to more blocks. 
    
    If the starting block was not already written, this function allocates, 
    links, and writes to more blocks.

    May also reassign the next_free_block if it had to allocate
    another block. The next_free_block will stall at the last block
    written when there are no more free blocks.

    Also assigns the `new_blocks_written` pointer.

    Forewards -1 but does not print if any errors are encountered by subroutines
    Otherwise silently returns 0 */

ERR read_raid_bytes_linked(BLOC_LOC starting_block, BYTE_LOC bytes_len, int8_t* bytes,
                           bool fixcorrupt, RAID raid);
/*  Reads the bytes along linked blocks begining at `starting_block` 
    until `bytes_len` bytes have been written to `bytes` or no more
    linked blocks are found.

    Does perform corruption detection and optionally correction

    Forewards -1 but does not print if any errors are encountered by subroutines
    Otherwise silently returns 0 */
```

### CLI

Most process invokations require specifying the `.jdf` files (extension not actually necesary) where the volume is located `DISKS= DISK=...`, raid arrangements `MIRRORS=0 CHAINS=1 STRIPED=false` and even more rarely, a user name `USER=`. 

```
man jfs

TODO: update this with final policies from jfs.h

NAME
    jfs - Jacob's File System

SYNOPSIS
    jfs {TODO: LIST ALL OPTIONS} arg=value...

DESCRIPTION
    The `jfs` CLI allows directly manipulating Jacob filesystems
    using the terminal or OS syscalls. Process arguments must have
    keyword identifiers (`ARG=VALUE`; overlapping names allowed).
    Unknown argument keys are silently ignored. Keys are not case
    sensitive. The CLI additionally supplies some default values 
    using profile information, environment variables, and constants.


jfs create NUM_BLOCKS= BLOCK_SIZE=256 VOLUME_NAME=disk DISKS= DISK=... MIRRORS=0 CHAINS=1 STRIPED=false

    Creates new disk(s) and then formats a new volume on those disks.
    Disks are equally sized and ceiling rounded if necesary. 
    
    Aliases: `createfs`.


jfs format DISKS= DISK=... VOLUME_NAME=disk.jfs BLOCK_SIZE=256 MIRRORS=0 CHAINS=1 STRIPED=false

    Formats disk(s). 
    
    Aliases: `formatfs`.


jfs combine NEWNAME= DISKS= DISK=... MIRRORS=1 CHAINS=1 STRIPED=false
    
    Combine all the disk(s) to a single disk file named NEWNAME. 
    
    Aliases: `savefs`.


jfs mount DISKS= DISK=... MIRRORS=0 CHAINS=1 STRIPED=false

    Updates the config file to remember what disk configuration
    is being used.
    
    Aliases: `openfs`


jfs unmount

    Removes disk configuration information from the config file.


jfs list START=/ META=true CONTENTS=false RECURSIVE=false DISKS= DISK=... MIRRORS=0 CHAINS=1 STRIPED=false USER=

    List paths (and optionally meta-information and file contents)
    starting from `path`. Optionally lists recursively and across
    symlinks.
    
    Aliases: `meta`, `metadata`.


jfs remove PATH= DISKS= DISK=... MIRRORS=0 CHAINS=1 STRIPED=false USER=

    Frees the space used by a single inode at PATH. Does not actually
    unlink the inode (unlinking is usually called after removing).
    

jfs delete PATH= RECURSIVE=true DISKS= DISK=... MIRRORS=0 CHAINS=1 STRIPED=false USER=

    Removes and unlinks an inode starting from PATH. Optionally
    deletes recursively and across symlinks.


jfs rename OLDPATH= NEWPATH= FIXSYMLINKS=true RECURSIVE=true DISKS= DISK=... MIRRORS=0 CHAINS=1 STRIPED=false USER=

    Renames / moves an inode from OLDPATH to NEWPATH. Optionally fixes
    symbolic links to newly moved inode. Optionally moves recursively
    (which allows entire directories to be moved) but does not move the
    symlinked files.
    
    Aliases: `move`.


jfs put_external_file EXTERNAL_FILEPATH= INTERNAL_FILEPATH= DISKS= DISK=... MIRRORS=0 CHAINS=1 STRIPED=false USER=

    Puts (stores) a single file from host OS into the jfs disk.
    Does not delete external file from host OS filesystem. 
    
    Aliases: `put`


jfs get_external_file INTERNAL_FILEPATH= EXTERNAL_FILEPATH= DISKS= DISK=... MIRRORS=0 CHAINS=1 STRIPED=false USER=

    Gets (retrieves) a single file from Jacob's file system
    to put on the host OS filesystem. Does not delete internal
    file from Jacob's filesystem. 

    Aliases: `get`


jfs set_user PATH= NEW_USER= RECURSIVE=true DISKS= DISK=... MIRRORS=0 CHAINS=1 STRIPED=false USER=

    Assigns the file at PATH to NEW_USER. Optionally changes ownership 
    recursively but not across symlinks (which allows entire directories
    to change ownership).

    Aliases `user`, `chown`. 


jfs link EXISTING_PATH= NEW_PATH= DISKS= DISK=... MIRRORS=0 CHAINS=1 STRIPED=false USER=

    Creates a link with path NEW_PATH to an inode at EXISTING_PATH.


jfs unlink PATH= DISKS= DISK=... MIRRORS=0 CHAINS=1 STRIPED=false USER=

    Unlinks an inode at PATH. Optionally recursively across
    symlinks and down directories.


jfs set_permissions USER_PERMISSIONS= ALL_PERMISSIONS= PATH= RECURSIVE= DISKS= DISK=... MIRRORS=0 CHAINS=1 STRIPED=false USER=

    Change permissions for an inode. USER_PERMISSIONS / ALL_PERMISSIONS
    are a single digit number 0-7 with standard UNIX bit flag meaning:
    (0=none, 1=only read, 2=only write, 3=read and write, 4=only exec,
    5=read and exec, 6=write and exec, and 7=read, write, and execute)
    Optionally sets permissions recursively, but not across symlinks.

    Aliases: `chmod`


jfs volume_info DISKS= DISK=... MIRRORS=0 CHAINS=1 STRIPED=false

    Get data contained in volume control block.
    
    Aliases: `vcb`. 


jfs new_user UNAME= DISKS= DISK=... MIRRORS=0 CHAINS=1 STRIPED=false

    Adds a new user
    

jfs remove_user UNAME= DISKS= DISK=... MIRRORS=0 CHAINS=1 STRIPED=false

    Removes a user


jfs mkdir PATH= DISKS= DISK=... MIRRORS=0 CHAINS=1 STRIPED=false USER=

    Makes a new directory at PATH.

```

**Main:**
1. get commmand
2. get config vars
    1. get all config vars from process arguments
        * `disk` appends to `disks`
    2. maybe add config vars from OS environment
    3. maybe add config vars from `~/.jfs_defaults`
    4. maybe add constant defaults
3. format config vars: for each config var:
    A. if it is a number: convert it to an `int64_t`
    B. if it is true or false: convert it to a boolean
    C. otherwise, leave it as a string
4. run appropriate library function with relevant argument variables
    * maybe use library utils to build RAIDINFO, RAID, and/or VCB
5. call `EXIT(SUCCESS)`

## Under the hood

Paths use UNIX style foreward-slash `/` directory addressing. *Directories* are *inodes* that list absolute paths to other inodes using '//' as a separator. (The root `/` is a directory.) Symbolic links only list one absolute path in their contents. Files can also be inodes. A single inode is stored across linked blocks (NULL terminal link). However a master inode table is used to store the full paths and start block number of all files. The metadata of an inode is stored in *attributes*. The *attribute table* lists key names, value bit length, inheritance, and the default value of all attributes. A value length limit of 0 means the attribute has an unknown (>=0) bit length terminated by 0 (such as a C-string). Both attribute keys and values can spill across multiple blocks. This allows file names to have arbitrary length 😊. Files must have `filesize` declared, and directories and symlinks must have `dests` declared, but otherwise attribute declaration is optional, and attribute values (such as permissions) can be inherited from their containing directory of drive. (Symlinks look like directories with one link.) `nobody` is the immutable user that owns the root. (See "attribute table" below for more details.) The `filedata` (not same as block's `data` section) contains the actual file contents. It is positioned after all attribute declarations, and contains `filesize` bits.

All blocks are structured as `[ xxxx xxx(valid=0/1) | data  (DATA_LEN) | (left padding if any) parity (N_PARITY) ]`. The parity is only computed from the data section. The term *disk* refers to all the bits of a Jacob Disk Format `.jdf` fille while *volume* refers to the data contained on a (or multiple) disks. A volume is hosted on RAIDs which are composed of optionally 1) mirrored, 2) chained, and/or 3) striped disks with operations performed in that order. Specifying `disks=disk0,disk1,disk2,you_have_to_type_it_all,disk58,disk59 chains=4, striped=true, mirrors=3` would 1) designate disks 0-19, 20-39, and 40-59 as mirror images, 2) form 4 chains in each mirror image (the first image would have disks 0-4, 5-9, 10-14, 15-19 form the chains), and finally, 3) stripe the volume data across each of those chains. RAID settings (along with future config settings) may be passed by arg in the CLI, read from the OS environment variables, or saved in `~/.jfs_defaults`. A single disk system denotes 1 single-disk-chain with no stripes or mirror. 

### Volume structure
```c
// lib/def.h

struct VCB_struct {
    int64_t block_size;
    int64_t jfs_format_version_major;
    int64_t jfs_format_version_minor;
    char* volume_name;
    int64_t datetime_last_formatted;
    BLOC_LOC total_space; // raw bytes
    BLOC_LOC max_possible_usable_space; // potentially usable bytes subtracting fs overhead
    BLOC_LOC free_space; // bytes
    BLOC_LOC free_blocks; // blocks
    BLOC_LOC num_files;
    BLOC_LOC num_dirs;
    BLOC_LOC num_symlinks;
    BLOC_LOC total_inodes;
    INODE* master_inode_table;
    BLOC_LOC next_free_block;
    int16_t num_reserved_chains;
    BLOC_LOC** reserved_chain_starting_blocs;
    int8_t num_users;
    char** unames;
};

// . . .

struct FILEATTR_struct {
    int8_t id;
    void* val;
};

// . . .

struct JFILE_struct {
    int8_t num_attrs;
    FILEATTR* attrs;
    byte* content;
};
```

### Compiler and Volume Constants
Code uses bits unless otherwise specified.

 - `N_PARITY = 8b`
 - `DATA_LEN = BLOCK_SIZE - 1b - N_PARITY`
 - `C_W = 1B` # later may use 4 byte width characters
 - `MAJOR_VERSION = 0`
 - `MINOR_VERSION = 0`
 - `LOC_POW = 40b`
 - `NUM_USR_POW = 8b`
 - `MAX_ATTRS_POW = 10b`

### Attribute table

Dirs have a `dests` attribute. 
Symlinks have a `links_to` attribute.
Files have a `file_size` attribute.

| Key | maxlen | inherited? | extra |
| --- | ------ | ---------- | ----------- |
| owner | 8b | `true` | `nobody` owns the root |
| file_size | 64b | `false` | only tabulates size of `filecontents` (not counting attributes) for inodes that store files (not directories or symlinks) |
| disk_space | 64b | `false` | tabulates in increments of `BLOCK_SIZE` |
| user_permissions | 8b | `true` | root has `rwx` |
| everybody_permissions | 8b | `true` | root has `rwx` |
| last_read_dt | 64b | `false` | `size_t` |
| last_write_dt |  64b | `false` | `size_t` |
| last_exec_dt |  64b | `false` | `size_t` |
| dests | 0 | `false` | C-string stores `/` separated paths for directories. |
| links_to | 0 | `false` | C-string for symlinks |
| incoming_links | 0 | `false` | C-string stores `/` separated paths to dirs and symlinks. |

### Configuration variables

These variables are used by the CLI. They may be passed as process / command line arguments, inherited from the OS environment, stored in `~/.jfs_config`, or default (ordered in decreasing precedence). The `disk` arg can be used repeatedly to append comma separate paths to `disks`.

| name | type | default | extra |
| ---- | ---- | -------------- | ----- |
| `disks` | `string` | (none) | comma separated (no space) filepaths (in the host OS) to each of the disk `.jdf` files. Example: `disk0.jdf,/tmp/mydisk,local_dir/disk2.bin` |
| `mirrors` | `int` | `0` | 0 means no  mirrors |
| `chains` | `int` | `1` |  |
| `striped` | `bool` | `false` |  |
| `user` | `string` | `nobody` |  |


## Getting Started

🛠️ NOTE: Only the core sections of the header library have been implemented. I planned too big for this project.

I recommend installation options by their degree of awesomeness (# of 😎's)

Please run all the following scripts from this project's root directory.

### Full System Installation  😎😎😎😎😎

```bash
sudo dev/install.sh --all  # ⚠️ not implemented
```

### Go Shell  😎😎😎😎

```bash
# install
sudo dev/install.sh --shell_go  # ⚠️ not implemented
jfs_shell_go

# just build and run
dev/compile_shell_go.sh  # ⚠️ not implemented
./jfs_shell_go
```

### C Shell  😎😎😎

```bash
# install
sudo dev/install.sh --shell_c  # ⚠️ not implemented
jfs_shell_c

# just build and run
dev/compile_shell_c.sh  # ⚠️ not implemented
./jfs_shell_c
```

### CLI  😎😎

```bash
# install
sudo dev/install.sh --cli  # ⚠️ not implemented

# just build and run
dev/compile_cli.sh  # ⚠️ not implemented
./jfs
```

### Just library  😎

```bash
# compile only:
dev/compile_lib.sh  # ⚠️ build passing on last commit
```


\* References to a jOS, jShell, jScript, jVM, jPhone, jPad, jPod, jMac, ... are presented amusingly and do not imply any real intention to build a larger computer ecosystem.