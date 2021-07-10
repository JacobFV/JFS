# ifndef JFS_HEADER
# define JFS_HEADER 1

#include "def.h"

// TODO all of the policies and mechanisms need to specify 
// how error codes are decided


error_t create(int64_t num_blocks, int32_t block_size, char* volume_name, RAIDINFO desired_raid_info);
/*  Policy: 
    Creates new disk(s) and then formats a new volume on those disks.
    Disks are equally sized and ceiling rounded if necesary. 

    Mechanism:
    1. create disk files
    2. use backend `format` to format new disk files */

error_t format(int32_t block_size, char* volume_name, RAID raid);
/*  Policy: 
    Formats disk(s). 

    Mechanism:
    1. use `write_raid` to propperly write 0's to from 0 to `num_blocks*block_size`
    2. write volume control block
        1. generate VCB in memory with `new_VCB`
        2. serialize VCB to bytes with `serialize_VCB`
        3. store byte-serialized VCB on raid with `write_data` */

error_t combine(char* new_name, RAID raid);
/*  Policy: 
    Combine all the disk(s) to a single disk file named NEWNAME. 

    1. use backend `create` to make a new single disk named `NEWNAME`
    2. taking the first disk option in case of mirror discrepenncies, 
        copy all bits raw onto to the new disk. */

error_t list(char* path, bool include_meta, bool include_contents, bool recursive, RAID raid, USER user);
/*  Policy: 
    List paths (and optionally meta-information and file contents)
    starting from `path`. Optionally lists recursively and across
    symlinks.

    Mechanism: 
    1. make sure the user has permission to read the inode at `path`
    2. print the absolute path of inode
    3. if include_meta is true: print the attributes of the inode
    4. if include_contents is true and the inode has a filesize attribute: 
        1. print the contents of the inode
    5. if the inode at start_path is a directory and recursive is true: 
        1. use `list` on all inodes in the directory. */

error_t remove(char* path, RAID raid, USER user);
/*  Policy: 
    Frees the space used by a single inode at PATH. Does not actually
    unlink the inode (unlinking is usually called after removing).

    Mechanism:
    1. make sure the user has permission to write to the inode at PATH
    2. edit volume information
        - increase vcb.free_usable_space
        - increase vcb.free_blocks
    3. assign all valid bits for the blocks used by the inode at PATH to false
    4. if unlink, also use `unlink` to unlink the path */

error_t delete(char* path, bool recursive, RAID raid, USER user);
/*  Policy:
    Removes and unlinks an inode starting from PATH. Optionally 
    deletes recursively and across symlinks.

    Mechanism:
    */

error_t rename(char* oldpath, char* newpath, bool fixsymlinks, bool recursive, RAID raid, USER user);
/*  Policy:
    Renames / moves an inode from OLDPATH to NEWPATH. Optionally fixes
    symbolic links to newly moved inode. Optionally moves recursively
    (which allows entire directories to be moved).

    Mechanism:
    1. make sure the user has permission to write to the inodes at OLDPATH and the containing directory of NEWPATH.
    2. for each inode with have a matching prefix pattern as oldpath in the master inode table:
        1. change path
        2. if the inode is a directory, rename the paths in its dests field.
        3. if fixsymlinks is true: rename the paths of all symlinks to this inode */


error_t put(char* external_filepath, char* internal_filepath, RAID raid, USER user);
/*  Policy:
    Puts (stores) a single file from host OS into the jfs disk.
    Does not delete external file from host OS filesystem. 

    Mechanism:
    1. make sure the user has permission to write to the inodes at INTERNAL_FILEPATH
    2. get file at external_path
    3. modify VCB parameters:
        - increment num files
        - increment num inodes
        -  TODO */

#### Get External File

```bash
jfs get_external_file INTERNAL_FILEPATH= EXTERNAL_FILEPATH= DISKS= DISK=... MIRRORS=0 CHAINS=1 STRIPED=false USER=
```

Get disk file, copy from “disk” to host OS file system. Does not delete the internal file from jfs. Aliases: `get`

#### Set User

```bash
jfs set_user PATH= USER= DISKS= DISK=... MIRRORS=0 CHAINS=1 STRIPED=false
```

Assigns the file at PATH to USER. Aliases `user`.

#### Link

```bash
jfs link EXISTING_PATH= NEW_PATH= DISKS= DISK=... MIRRORS=0 CHAINS=1 STRIPED=false USER=
```

Creates a hard link with path NEW_PATH to an inode at EXISTING_PATH.

#### Unlink

```bash
jfs unlink PATH= DISKS= DISK=... MIRRORS=0 CHAINS=1 STRIPED=false USER=
```

Unlinks an inode at PATH.

**backend unlink**
1. make sure the user has permission to write to this file
2. if the inode has no incoming symlinks:
    1. remove inode from master inode table
    2. decrease vcb.total_inodes
    3. if fixsymlinks is true: rename the paths of all symlinks to this inode
    4. if inode is file: decrease vcb.num_files
    5. if the inode is a symlink: remove that symlink from the `dests` attribute of the linked file
    6. if inode is directory: 
        1. decrease vcb.num_directories
        2. use `unlink` on all files in the directory

#### Change Permissions

```bash
jfs chmod USER_PERMISSIONS= ALL_PERMISSIONS= PATH= DISKS= DISK=... MIRRORS=0 CHAINS=1 STRIPED=false USER=
```

Change permissions for a file. *_PERMISSIONS are a single digit number 0-7 with standard UNIX meaning (0=none, 1=only read, 7=read, write, and execute)

#### Detect Corruption

```bash
jfs detect_corruption PATH= RECURSIVE=false TRYFIX=false DISKS= DISK=... MIRRORS=0 CHAINS=1 STRIPED=false USER=
```

Detect corruption with the parity bit and conditionally correct if a single bit flip.

#### Volume Info

```bash
jfs volume_info DISKS= DISK=... MIRRORS=0 CHAINS=1 STRIPED=false
```

Get data contained in volume control block. Aliases: `volume`. 



# endif /* JFS_HEADER */