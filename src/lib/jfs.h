# ifndef JFS_HEADER
# define JFS_HEADER 1

#include "def.h"


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

error_t format(int32_t block_size, char* volume_name, RAID raid);
/*  Policy: 
    Formats disk(s). 

    Mechanism:
    1. use `write_raid_bytes_raw` to propperly write 0's 
        from 0 to `num_blocks*block_size`
    2. write volume control block
        1. generate VCB in memory with `new_VCB`
        2. serialize VCB to bytes with `save_VCB`
    3. use `print_error` to print and also return
        error codes encountered */

error_t combine(char* new_name, RAID raid);
/*  Policy: 
    Combine all the disk(s) to a single disk file named NEWNAME. 

    1. use backend `create` to make a new single disk named `NEWNAME`
    2. copy all bits raw onto to the new disk.
        1. read and write blocks sequentially
    3. use `print_error` to print and also return
        error codes encountered */

error_t list(char* path, bool include_meta, 
             bool include_contents, bool recursive, 
             VCB vcb, RAID raid, USER user);
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
    6. use `print_error` to print and also return
        error codes encountered */

error_t remove(char* path, 
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
    5. use `print_error` to print and also return error codes encountered */

error_t delete_inode(char* path, bool recursive, 
                     VCB vcb, RAID raid, USER user);
/*  Policy:
    Removes and unlinks an inode starting from PATH. Optionally 
    deletes recursively and across symlinks.

    Mechanism:
    1. use `authenticate` to ensure sure the user has permission 
        - to write to the inode at path
        - to write to the containing directory
    2. use `remove` to remove the inode at `path` (not recursively)
    3. use `unlink` to unlink the inode at `path` (not recursively)
    4. if recursive is true:
        A. if the inode was a directory:
            1. use `delete_inode` on all its children (recursively)
        B. if the inode was a symlink:
            1. use `delete_inode` on all its linked inode (recursively)
    5. use `print_error` to print and also return error codes encountered */

error_t rename(char* oldpath, char* newpath, 
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
    3. use `print_error` to print and also return error codes encountered */

error_t put(char* external_filepath, char* internal_filepath, 
            VCB vcb, RAID raid, USER user);
/*  Policy:
    Puts (stores) a single file from host OS into the jfs disk.
    Does not delete external file from host OS filesystem. 

    Mechanism:
    1. use `authenticate` to ensure the user has permission to write 
        to the directory containing internal_filepath
    2. get file at external_path from OS
    3. use `create_inode_and_jfile` to add file to jfs
    4. use `print_error` to print and also return error codes encountered */

error_t get(char* internal_filepath, char* external_filepath, 
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
    5. use `print_error` to print and also return error codes encountered */

error_t set_user(char* path, char* new_user, bool recursive, 
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
    6. use `print_error` to print and also return error codes encountered */

error_t link(char* existing_path, char* new_path, 
             VCB vcb, RAID raid, USER user);
/*  Policy:
    Creates a link with path new_path to an inode at existing_path.

    Mechanism:
    1. use `authenticate` to ensure the user has permission 
        to write to new_path
        to write to the directory containing new_path
    2. use `create_inode_and_jfile` to make a jfile at new_path
        with attribute "links_to"=existing_path 
    3. add this link to the incoming links at new_path
        1. open jfile
        2. append path to symlink
        3. save jfile
    4. use `print_error` to print and also return error codes encountered */

error_t unlink(char* path, bool recursive,
               VCB vcb, RAID raid, USER user);
/*  Policy:
    Unlinks an inode at path.

    Mechanism:
    1.  use `authenticate` to ensure the user has permission 
        - to write to the inode at path
        - to write to the containing directory
    2. if the inode has no incoming symlinks: 
    (this keeps a file alive if it is referred to by two paths and one of them gets deleted)
        2A. if inode is a file: 
            1. decrement vcb num_files
        2B. if the inode is a symlink: 
            1. decrement vcb num_symlinks
        2C. if inode is a directory: 
            1. decrement vcb num_dirs
            2. if recursive is true:
                1. use `unlink` on all files in the directory
        3. remove inode from master inode table
        4. decrement vcb total_inodes
    3. use `print_error` to print and also return error codes encountered */

error_t set_permissions(int8_t user_permissions, int8_t all_permissions, 
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
    6. use `print_error` to print and also return error codes encountered */

error_t corruption(char* path, bool recursive, bool tryfix, 
                   VCB vcb, RAID raid, USER user);
/*  Policy:
    Detect corruption in blocks holding data for file at PATH.
    Optionally checks for corruption recursively and across symlinks.
    Optionally corrects single bit error using parity bits.

    Mechanism:
    1. foreward errors from `read_raid_bytes_linked` starting at
        the first block for inode at path
    2. use `print_error` to print and also return error codes encountered */

error_t volume_info(VCB vcb);
/*  Policy: 
    Get data contained in volume control block.

    Mechanism: 
    1. for every field of VCB in the order declared
        1. print field value
    2. use `print_error` to print and also 
        return error codes encountered */


error_t new_user(char* uname, VCB vcb, RAID raid);
/*  Policy:
    Adds a new user

    Mechanism:
    1. increment vcb num_users
    2. add user to vcb users
    3. use `print_error` to print and also 
        return error codes encountered */

error_t remove_user(char* uname, VCB vcb, RAID raid);
/*  Policy:
    Removes a user

    Mechanism:
    1. look for and remove user from vcb users
    2. decrement vcb num_users
    3. use `print_error` to print and also 
        return error codes encountered */

error_t mkdir(char* path, 
              VCB vcb, RAID raid, USER user);
/*  Policy:
    Makes a new directory at path.

    Mechanism:
    1. use `authenticate` to ensure the user has permission to write 
        to the directory containing path
    2. use `create_inode_and_jfile` to add an empty dir to jfs
    3. use `print_error` to print and also 
        return error codes encountered */


# endif /* JFS_HEADER */