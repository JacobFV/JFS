#include "vcb.h"


VCB new_VCB()
/*  Allocates and initializes a new VCB. */
{
    VCB vcb = malloc(sizeof(VCB));

    vcb->jfs_format_version_major = 1;
    vcb->jfs_format_version_minor = 0;

    return vcb;
}

ERR load_VCB(VCB* vcb_ptr, RAID raid)
/*  Loads VCB from raid 

    Forewards -1 if any errors are encountered by subroutines
    Otherwise silently returns 0 */
{
    VCB vcb = new_VCB();
    vcb_ptr = &vcb;
    
    int64_t N = 0;
    vcb->block_size = parse_int64(N, raid);
    N += 8;
    vcb->jfs_format_version_major = parse_int64(N, raid);
    N += 8;
    vcb->jfs_format_version_major = parse_int64(N, raid);
    N += 8;
    vcb->volume_name = parse_string(N, raid, &N);
    // N automatically incremented
    vcb->datetime_last_formatted = parse_int64(N, raid);
    N += 8;
    vcb->total_space = parse_int64(N, raid);
    N += 8;
    vcb->max_possible_usable_space = parse_int64(N, raid);
    N += 8;
    vcb->free_space = parse_int64(N, raid);
    N += 8;
    vcb->free_blocks = parse_int64(N, raid);
    N += 8;
    vcb->num_files = parse_int64(N, raid);
    N += 8;
    vcb->num_dirs = parse_int64(N, raid);
    N += 8;
    vcb->num_symlinks = parse_int64(N, raid);
    N += 8;
    vcb->total_inodes = parse_int64(N, raid);
    N += 8;
    vcb->master_inode_table = calloc(vcb->total_inodes, sizeof(INODE));
    for(int i = 0; i < vcb->total_inodes; i++) {
        INODE inode = malloc(sizeof(INODE));
        inode->path = parse_string(N, raid, &N);
        inode->start_block = parse_int64(N, raid);
        N += 8;
        vcb->master_inode_table[i] = inode;
    }
    vcb->next_free_block = parse_int64(N, raid);
    N += 8;
    vcb->num_users = parse_int8(N, raid);
    N += 1;
    vcb->unames = calloc(vcb->num_users, sizeof(char*));
    for(int i = 0; i < vcb->num_users; i++) {
        vcb->unames[i] = parse_string(N, raid, &N);
    }
    
    return 0;
}

ERR save_VCB(VCB vcb, RAID raid)
/*  Saves vcb to raid

    Forewards -1 if any errors are encountered by subroutines
    Otherwise silently returns 0 */
{
    int N = 0;
    save_int64(&N, vcb->block_size, raid);
    save_int64(&N, vcb->jfs_format_version_major, raid);
    save_int64(&N, vcb->jfs_format_version_minor, raid);
    save_string(&N, vcb->volume_name);
    save_int64(&N, vcb->total_space, raid);
    save_int64(&N, vcb->max_possible_usable_space, raid);
    save_int64(&N, vcb->free_space, raid);
    save_int64(&N, vcb->free_blocks, raid);
    save_int64(&N, vcb->num_files, raid);
    save_int64(&N, vcb->num_dirs, raid);
    save_int64(&N, vcb->num_symlinks, raid);
    save_int64(&N, vcb->total_inodes, raid);
    for(int i = 0; i < vcb->total_inodes; i++) {
        save_string(&N, vcb->master_inode_table[i]->path);
        save_int64(&N, vcb->master_inode_table[i]->start_block);
    }
    save_int64(&N, vcb->next_free_block, raid);
    save_int64(&N, vcb->num_users, raid);
    for(int i = 0; i < vcb->num_users; i++) {
        save_string(&N, vcb->unames[i]);
    }

    return 0;
}