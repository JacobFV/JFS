#include "bits.h"
#include "jfs.h"
#include "bits.h"
#include "libutils.h"
#include "diskutils.h"
#include "vcb.h"
#include "fsutils.h"

ERR create(int64_t num_blocks, int32_t block_size, 
           char* volume_name, RAIDINFO desired_raid_info)
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
{
    int err;
    int64_t total_bytes = num_blocks * block_size;

    // TODO make sure desired_raid_info is correct: whole number division of mirrors, etc

    FILE* fp;
    for(int i = 0; i < desired_raid_info->num_disks; i++) {
        fp = fopen(desired_raid_info->paths[i], "w");
        err = fclose(fp);
        if(err==-1) return -1;
    }

    RAID raid;
    err = open_RAID(desired_raid_info, &raid);
    if(err==-1) return -1;

    err = format(num_blocks, block_size, volume_name, raid);
    if(err==-1) return -1;

    free_RAID(raid);

    return 0;
}

ERR format(int64_t num_blocks, int32_t block_size, char* volume_name, RAID raid)
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
{
    int err;
    int64_t total_bytes = num_blocks*block_size;

    byte* bytes = calloc(total_bytes, sizeof(byte));
    err = write_raid_bytes_raw(0, total_bytes, bytes, raid);
    if(err==-1) return -1;
    free(bytes);

    VCB vcb = new_VCB();
    vcb->volume_name = volume_name;
    vcb->block_size = block_size;
    vcb->total_space = total_bytes;
    err = free_VCB(vcb, raid);
    if(err==-1) return -1;

    return 0;
}

ERR combine(char* new_name, RAID raid)
/*  Policy: 
    Combine all the disk(s) to a single disk file named NEWNAME. 

    1. use backend `create` to make a new single disk named `NEWNAME`
    2. copy all bits raw onto to the new disk.
        1. read and write blocks sequentially

    Returns -1 and prints error if newly created disk does not exactly match
        byte capacity of old raid
    Forewards -1 but does not print if any errors are encountered by subroutines
    Otherwise silently returns 0 */
{
    int err;

    VCB old_vcb;
    err = load_VCB(&old_vcb, raid);
    if(err==-1) return -1;

    RAIDINFO newraid_info = new_raidinfo(1, 1, 1, 1, &new_name);
    err = create(
        old_vcb->total_space / old_vcb->block_size,
        old_vcb->block_size,
        new_name,
        new_raidinfo
    );
    if(err==-1) return -1;

    RAID newraid;
    err = open_RAID(new_raidinfo, newraid);
    if(err==-1) return -1;

    VCB new_vcb;
    err = load_VCB(&new_vcb, newraid);
    if(err==-1) return -1;

    if(old_vcb->total_space != new_vcb->total_space) {
        fprintf(stderr, "The newly created disk does not exactly match the byte amount of the old raid");
        return -1;
    }

    byte* bytes = calloc(new_vcb->block_size, sizeof(byte));
    for(int i = 0; i < new_vcb->total_space / new_vcb->block_size; i++) {
        err = read_raid_bytes_raw(0, new_vcb->total_space, bytes, raid);
        if(err==-1) return -1;

        err = write_raid_bytes_raw(0, new_vcb->total_space, bytes, newraid);
        if(err==-1) return -1;
    }

    free(bytes);
    free(old_vcb);
    free(newraid_info);
    free(newraid);
    free(new_vcb);

    return 0;
}

ERR list(char* path, bool include_meta, 
         bool include_contents_hex, bool include_contents_char,
         bool recursive, VCB vcb, RAID raid, USER user)
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
{
    int err;

    JFILE jfile;
    err = open_jfile(path, true, &jfile, vcb, raid);
    if(err==-1) return -1;

    if(!authenticate(path, jfile, true, false, false, user, vcb)) return 0;

    printf("%s\n", path);

    if(include_meta) {
        printf("attributes:\n");
        char* attrname;
        for(int i = 0; i < jfile->num_attrs; i++) {
            printf(" %s: ", attr_name_from_id(jfile->attrs[i]->id));

            switch (attrtype_from_id(jfile->attrs[i]->id))
            {
            case STRING_ATTRTYPE:
                printf("%i\n", *( (char**)(jfile->attrs[i]->val) ));
                break;
            case INT8_ATTRTYPE:
                printf("%i\n", *( (int8_t*)(jfile->attrs[i]->val) ));
                break;
            case INT16_ATTRTYPE:
                printf("%i\n", *( (int16_t*)(jfile->attrs[i]->val) ));
                break;
            case INT32_ATTRTYPE:
                printf("%i\n", *( (int32_t*)(jfile->attrs[i]->val) ));
                break;
            case INT64_ATTRTYPE:
                printf("%i\n", *( (int64_t*)(jfile->attrs[i]->val) ));
                break;
            default: 
                // TODO: document this error case
                fprintf(stderr, "unexpected attribute type: |%i|\n", 
                        attrtype_from_id(jfile->attrs[i]->id));
                return -1;
            }
        }
    }

    if(include_contents_hex && jfile->inode_type == FILE_JFILE_TYPE) {
        printf("contents (hex):\n");
        for(int i = 0; i < jfile->filesize; i++) {
            printf("%x ", jfile->content[i]);
        }
        printf("\n");
    }

    if(include_contents_char && jfile->inode_type == FILE_JFILE_TYPE) {
        printf("contents (char):\n");
        for(int i = 0; i < jfile->filesize; i++) {
            putchar(jfile->content[i]);
        }
        printf("\n");
    }

    if(recursive) {
        switch (jfile->inode_type)
        {
        case DIR_JFILE_TYPE:
            int num_children;
            char** child_paths = separate_path_list(jfile->dests, &num_children);

            for(int i = 0; i < num_children; i++) {
                list(child_paths[i], include_meta, 
                     include_contents_hex, include_contents_char,
                     recursive, vcb, raid, user);
                free(child_paths[i]);
            }
            free(child_paths);
            break;
        
        case SYMLINK_JFILE_TYPE:
            list(jfile->links_to, include_meta, 
                    include_contents_hex, include_contents_char,
                    recursive, vcb, raid, user);
            break;
        
        default:
            break;
        }
    }

    free_JFILE(jfile, vcb, raid);

    return 0;
}

ERR remove(char* path, JFILE jfile,
           VCB vcb, RAID raid, USER user)
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
{
    int err;
    if(!authenticate(path, jfile, true, false, false, user, vcb)) return 0;

    int64_t filespace;
    free(serialize_file(path, jfile, &filespace, vcb, raid));
    vcb->free_space += filespace;

    BLOC_LOC bloc, starting_bloc = jfile->starting_bloc;

    err = free_JFILE(jfile, vcb, raid);
    if(err==-1) return -1;

    byte* bytes = calloc(8, sizeof(byte));
    while(bloc) {
        vcb->free_blocks++;
        bytes[0] = 0;

        err = write_raid_bytes_raw(vcb->block_size*bloc, 1, bytes, raid);
        if(err==-1) return -1;
        err = read_raid_bytes_raw(vcb->block_size*bloc, 8, bytes, raid);
        if(err==-1) return -1;

        bloc = bytes[0] |
               bytes[1] << 8 |
               bytes[2] << 16 |
               bytes[3] << 24 |
               bytes[4] << 32 |
               bytes[5] << 40 |
               bytes[6] << 48 |
               bytes[7] << 56;
    }
    free(bytes);

    vcb->next_free_block = (starting_bloc < vcb->next_free_block) 
                            ? starting_bloc : vcb->next_free_block;

    return 0;
}

ERR delete_inode(char* path, bool recursive, 
                 VCB vcb, RAID raid, USER user)
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
{
    int err;

    if(strlen(path) <= 1) {
        printf("cannot delete root\n");
        return -1; // TODO error documentation
    }

    JFILE jfile;
    err = open_jfile(path, true, &jfile, vcb, raid);
    if(err==-1) return -1;

    JFILE jdir;
    char* dirpath = calloc(strlen(path) + 1, sizeof(char));
    strncpy(dirpath, path, parent_slash_idx(path));
    err = open_jfile(dirpath, true, &jdir, vcb, raid);
    if(err==-1) return -1;

    if(!authenticate(path, jfile, false, true, false, user, vcb)) return 0;
    if(!authenticate(dirpath, jdir, false, true, false, user, vcb)) return 0;

    if(recursive) {
        switch (jfile->inode_type)
        {
        case DIR_JFILE_TYPE:
            int num_children;
            char** child_paths = separate_path_list(jfile->dests, &num_children);

            for(int i = 0; i < num_children; i++) {
                delete_inode(child_paths[i], recursive, vcb, raid, user);
                free(child_paths[i]);
            }
            free(child_paths);
            break;
        
        case SYMLINK_JFILE_TYPE:
                delete_inode(jfile->links_to, recursive, vcb, raid, user);
            break;
        
        default:
            break;
        }
    }

    err = remove(path, jfile, vcb, raid, user);
    if(err==-1) return -1;
    err = unlink(path, false, vcb, raid, user);
    if(err==-1) return -1;

    // jfile freed by `remove`
    free_JFILE(jdir, vcb, raid);
    free(dirpath);

    return 0;
}

ERR rename(char* oldpath, char* newpath, 
           bool fixsymlinks, bool recursive,
           VCB vcb, RAID raid, USER user)
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
{

    int err;

    if(fixsymlinks) {
        printf("jfs 1.0 does not fix symbolic links when moving");
        return -1;
    }

    if(strlen(oldpath) <= 1) {
        printf("cannot delete root\n");
        return -1; // TODO error documentation
    }

    JFILE jfile;
    err = open_jfile(oldpath, true, &jfile, vcb, raid);
    if(err==-1) return -1;

    JFILE oldjdir;
    char* olddirpath = calloc(strlen(oldpath) + 1, sizeof(char));
    strncpy(olddirpath, oldpath, parent_slash_idx(oldpath));
    err = open_jfile(olddirpath, true, &oldjdir, vcb, raid);
    if(err==-1) return -1;

    JFILE newjdir;
    char* newdirpath = calloc(strlen(newpath) + 1, sizeof(char));
    strncpy(newdirpath, newpath, parent_slash_idx(newpath));
    err = open_jfile(newdirpath, true, &newjdir, vcb, raid);
    if(err==-1) return -1;

    if(!authenticate(oldpath, jfile, false, true, false, user, vcb)) return 0;
    if(!authenticate(newdirpath, newjdir, false, true, false, user, vcb)) return 0;

    // first copy regular file
    for(int i = 0; i < vcb->total_inodes; i++) {
        if(strcmp(vcb->master_inode_table[i]->path, oldpath) == 0) {
            free(vcb->master_inode_table[i]->path);
            char* newnamecopy = calloc(strlen(newpath)+1, sizeof(char));
            strcpy(vcb->master_inode_table[i]->path, newpath);
            break;
        }
    }

    // then maybe copy recursively
    if(recursive && jfile->inode_type == DIR_JFILE_TYPE) {

        char* oldpathptr;
        size_t newsize;
        for(int i = 0; i < vcb->total_inodes; i++) {
            if(strncmp(vcb->master_inode_table[i]->path, oldpath, strlen(oldpath)) == 0) {

                oldpathptr = vcb->master_inode_table[i]->path;
                newsize = strlen(newpath) 
                        + (strlen(oldpathptr) 
                        - strlen(oldpath))
                        + 1;
                vcb->master_inode_table[i]->path = calloc(newsize, sizeof(char));
                strcpy(vcb->master_inode_table[i]->path, newpath);
                strcpy(vcb->master_inode_table[i]->path, oldpathptr+strlen(newpath));
                free(oldpathptr);
            }
        }
    }

    free_JFILE(jfile, vcb, raid);
    free_JFILE(oldjdir, vcb, raid);
    free_JFILE(newjdir, vcb, raid);
    free(olddirpath);
    free(newdirpath);

    return 0;
}

ERR put(char* external_filepath, char* internal_filepath, 
        VCB vcb, RAID raid, USER user)
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
{
    int err;

    if(strlen(internal_filepath) == 0) {
        printf("must specify an internal_filepath\n");
        return -1; // TODO error documentation
    }

    JFILE jdir;
    char* internal_dirpath = calloc(parent_slash_idx(internal_filepath)+1, sizeof(char));
    err = open_jfile(internal_dirpath, true, &jdir, vcb, raid);
    if(err==-1) return -1;

    if(!authenticate(internal_dirpath, jdir, false, true, false, user, vcb)) return 0;

    FILE* fp = fopen(external_filepath, "r");

    err = create_inode_and_jfile(internal_filepath, fp, 0, NULL, user, vcb, raid);
    if(err==-1) return -1;

    err = fclose(fp);
    if(err==-1) return -1;

    free(internal_filepath);
    return 0;
}

ERR get(char* internal_filepath, char* external_filepath, 
        VCB vcb, RAID raid, USER user)
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
{
    int err;

    JFILE jfile;
    err = open_jfile(internal_filepath, true, &jfile, vcb, raid);
    if(err==-1) return -1;

    if(!authenticate(internal_filepath, jfile, false, true, false, user, vcb)) return 0;

    FILE* fp = fopen(external_filepath, "w");

    if(jfile->inode_type == FILE_JFILE_TYPE) {
        for(int i = 0; i < jfile->filesize; i++) {
            fputc(jfile->content[i], fp);
        }
    }
    else {
        const int simple_data_size = vcb->block_size-1-N_PARITY; // BUGGY
        byte* bytes = calloc(simple_data_size, sizeof(byte));
        read_raid_bytes_linked(jfile->starting_bloc, simple_data_size,
                               bytes, true, raid);
        for(int i = 0; i < simple_data_size; i++) {
            fputc(bytes[i], fp);
        }
        free(bytes);
    }

    err = fclose(fp);
    if(err==-1) return -1;

    err = free_JFILE(jfile, vcb, raid);
    if(err==-1) return -1;

    return 0;
}

ERR set_user(char* path, USER new_user, bool recursive, 
             VCB vcb, RAID raid, USER user)
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
{
    int err;

    JFILE jfile;
    err = open_jfile(path, true, &jfile, vcb, raid);
    if(err==-1) return -1;

    if(!authenticate(path, jfile, false, true, false, user, vcb)) return 0;

    for(int i = 0; i < jfile->num_attrs; i++) {
        if(jfile->attrs[i]->id == OWNER_ATTR_ID) {
            *((int8_t*)(jfile->attrs[i]->val)) = new_user;
            break;
        }
    }

    if(recursive && jfile->inode_type == DIR_JFILE_TYPE) {
        int num_children;
        char** child_paths = separate_path_list(jfile->dests, &num_children);
        for(int i = 0; i < num_children; i++) {
            err = set_user(child_paths[i], new_user, recursive, vcb, raid, user);
            if(err==-1) return -1;
            free(child_paths[i]);
        }
        free(child_paths);
    }

    err = free_JFILE(jfile, vcb, raid);
    if(err==-1) return -1;

    return 0;
}

ERR link(char* existing_path, char* new_path, 
         VCB vcb, RAID raid, USER user)
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
{
    int err;

    if(strlen(new_path) == 0) {
        printf("must specify a new_path\n");
        return -1; // TODO error documentation
    }

    JFILE jdir;
    char* new_dirpath = calloc(parent_slash_idx(new_path)+1, sizeof(char));
    err = open_jfile(new_dirpath, true, &jdir, vcb, raid);
    if(err==-1) return -1;
    free(new_dirpath);

    if(!authenticate(new_dirpath, jdir, false, true, false, user, vcb)) return 0;

    FILEATTR fattr = new_fileattr(LINKS_TO_ATTR_ID, &new_path);
    err = create_inode_and_jfile(new_path, NULL, 1, &fattr, user, vcb, raid);
    if(err==-1) return -1;

    free_JFILE(jdir, vcb, raid);

    return 0;
}


ERR unlink(char* path, bool recursive,
           VCB vcb, RAID raid, USER user)
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
{
    int err;

    JFILE jfile;
    err = open_jfile(path, true, &jfile, vcb, raid);
    if(err==-1) return -1;

    JFILE jdir;
    char* dirpath = calloc(parent_slash_idx(path)+1, sizeof(char));
    err = open_jfile(dirpath, true, &jdir, vcb, raid);
    if(err==-1) return -1;

    char* incoming_symlinks;
    for(int i = 0; i < jfile->num_attrs; i++) {
        if(jfile->attrs[i]->id == INCOMING_LINKS_ATTR_ID) {
            incoming_symlinks = *((char**)(jfile->attrs[i]->val));
        }
    }
    int num_symlinks;
    char** symlinks = separate_path_list(incoming_symlinks, &num_symlinks);
    JFILE sl;
    for(int i = 0; i < num_symlinks; i++) {
        err = open_jfile(symlinks[i], true, &sl, vcb, raid);
        if(err==-1) return -1;
        if(!authenticate(symlinks[i], sl, false, true, false, user, vcb));
        delete_inode(symlinks[i], false, vcb, raid, user);
        free_JFILE(sl, vcb, raid);
        free(symlinks[i]);
    }
    free(symlinks);

    if(!authenticate(path, jfile, false, true, false, user, vcb)) return 0;
    if(!authenticate(dirpath, jdir, false, true, false, user, vcb)) return 0;

    switch(jfile->inode_type) {
    case FILE_JFILE_TYPE:
        vcb->num_files--;
        break;
    case SYMLINK_JFILE_TYPE:
        vcb->num_symlinks--;
        break;
    case DIR_JFILE_TYPE:
        vcb->num_dirs--;
        if(recursive) {
            int num_children;
            char** child_paths = separate_path_list(jfile->dests, &num_children);
            for(int i = 0; i < num_children; i++) {
                unlink(child_paths[i], recursive, vcb, raid, user);
                free(child_paths[i]);
            }
            free(child_paths);
        }
        break;
    }

    bool found = false;
    for(int i = 0; i < vcb->total_inodes; i++) {
        if(!found) {
            if(strcmp(vcb->master_inode_table[i]->path, path) == 0) {
                free(vcb->master_inode_table[i]->path);
                free(vcb->master_inode_table[i]);
                found = true;
            }
        }
        else {
            vcb->master_inode_table[i-1] = vcb->master_inode_table[i];
        }
    }
    vcb->total_inodes--;

    char* dests_str_ptr;
    for(int i = 0; i < jdir->num_attrs; i++) {
        if(jdir->attrs[i]->id == DESTS_ATTR_ID) {
            dests_str_ptr = ((char**)(jdir->attrs[i]->val));
        }
    }
    int num_dests;
    char** dests_split = separate_path_list(*dests_str_ptr, &num_dests);
    for(int i = 0; i < num_dests; i++) {
        if(strcmp(dests_split[i], path) == 0) {
            strcpy(dests_split[i], "");
        }
    }
    free(*dests_str_ptr);
    *dests_str_ptr = combine_path_list(dests_split, num_dests);
    free(dests_split);

    err = free_JFILE(jfile, vcb, raid);
    if(err==-1) return -1;
    err = free_JFILE(jdir, vcb, raid);
    if(err==-1) return -1;
    free(dirpath);

    return 0;
}

ERR set_permissions(int8_t user_permissions, int8_t all_permissions, 
                    char* path, bool recursive, 
                    VCB vcb, RAID raid, USER user)
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
{
    int err;

    JFILE jfile;
    err = open_jfile(path, true, &jfile, vcb, raid);
    if(err==-1) return -1;

    for(int i = 0; i < jfile->num_attrs; i++) {
        switch (jfile->attrs[i]->id)
        {
        case USER_PERMISSIONS_ATTR_ID:
            *((int8_t*)jfile->attrs[i]->val) = user_permissions;
            break;
        case ALL_PERMISSIONS_ATTR_ID:
            *((int8_t*)jfile->attrs[i]->val) = all_permissions;
            break;
        case DESTS_ATTR_ID:
            int num_children;
            char** child_paths = separate_path_list(jfile->dests, &num_children);

            for(int i = 0; i < num_children; i++) {
                set_permissions(user_permissions, all_permissions,
                    child_paths[i], recursive, vcb, raid, user);
                free(child_paths[i]);
            }
            free(child_paths);
            break;
        
        default:
            break;
        }
    }

    free_JFILE(jfile, vcb, raid);

    return 0;
}

ERR volume_info(VCB vcb)
/*  Policy: 
    Get data contained in volume control block.

    Mechanism: 
    1. for every field of VCB in the order declared
        1. print field value 
        
    Forewards -1 but does not print if any errors are encountered by subroutines
    Otherwise silently returns 0 */
{
    printf("block_size:                 %d", vcb->block_size);
    printf("jfs_format_version_major:   %d", vcb->jfs_format_version_major);
    printf("jfs_format_version_minor:   %d", vcb->jfs_format_version_minor);
    printf("volume_name:                %s", vcb->volume_name);
    printf("datetime_last_formatted:    %d", vcb->datetime_last_formatted);
    printf("total_space:                %d", vcb->total_space);
    printf("max_possible_usable_space:  %d", vcb->max_possible_usable_space);
    printf("free_space:                 %d", vcb->free_space);
    printf("free_blocks:                %d", vcb->free_blocks);
    printf("num_files:                  %d", vcb->num_files);
    printf("num_dirs:                   %d", vcb->num_dirs);
    printf("num_symlinks:               %d", vcb->num_symlinks);
    printf("total_inodes:               %d", vcb->total_inodes);
    printf("...     (use `list recursive=true` to see all inodes)");
    printf("next_free_block:            %d", vcb->next_free_block);
    printf("num_reserved_chains:        %d", vcb->num_reserved_chains);
    for(int i = 0; i < vcb->num_reserved_chains; i++) {
        printf("  reserved chain %d starts at %d", 
               i, vcb->reserved_chain_starting_blocs[i]);
    }
    printf("num_users:                  %d", vcb->num_users);
    for(int i = 0; i < vcb->num_users; i++) {
        printf("  uid: %d   uname: %s \n", i, vcb->unames[i]);
    }

    return 0;
}

ERR new_user(char* uname, VCB vcb)
/*  Policy:
    Adds a new user

    Mechanism:
    1. increment vcb num_users
    2. add user to vcb users 

    Forewards -1 but does not print if any errors are encountered by subroutines
    Otherwise silently returns 0 */ 
{
    char* uname_save = calloc(strlen(uname) + 1, sizeof(char));
    strcpy(uname_save, uname);

    char** old_unames = vcb->unames;
    vcb->unames = calloc(vcb->num_users, sizeof(char*));
    memcpy(vcb->unames, old_unames, vcb->num_users);
    free(old_unames);
    vcb->num_users++;
    vcb->unames[vcb->num_users] = uname_save;

    return 0;
}

ERR remove_user(char* uname, VCB vcb)
/*  Policy:
    Removes a user

    Mechanism:
    1. look for and remove user from vcb users
    2. decrement vcb num_users

    Forewards -1 but does not print if any errors are encountered by subroutines
    Otherwise silently returns 0  */
{
    bool found = false;
    for(int i = 0; i < vcb->num_users; i++) {
        if(!found) {
            if(strcmp(vcb->unames[i], uname) == 0) {
                free(vcb->unames[i]);
                found = true;
            }
        }
        else {
            vcb->unames[i-1] = vcb->unames[i];
        }
    }
    vcb->num_users--;

    return 0;
}

ERR mkdir(char* path, VCB vcb, RAID raid, USER user)
/*  Policy:
    Makes a new directory at path.

    Mechanism:
    1. use `authenticate` to ensure the user has permission to write 
        to the directory containing path
    2. use `create_inode_and_jfile` to add an empty dir to jfs 

    Forewards -1 but does not print if any errors are encountered by subroutines
    Otherwise silently returns 0 */
{
    int err;

    JFILE jdir;
    char* dirpath = calloc(parent_slash_idx(path)+1, sizeof(char));
    err = open_jfile(dirpath, true, &jdir, vcb, raid);
    if(err==-1) return -1;

    if(!authenticate(dirpath, jdir, false, true, false, user, vcb));

    char* empty_string = calloc(1, sizeof(char));
    FILEATTR fattr = new_fileattr(DESTS_ATTR_ID, empty_string);
    err = create_inode_and_jfile(path, NULL, 1, &fattr, user, vcb, raid);
    if(err==-1) return -1;

    return 0;
}
