# jFS

Jacob's file system (jFS) is a file system that can be invoked directly from the operating system. It currently requires an OS-provided filesystem substrate, but a future operating system jOS will directly utilize jFS to store data on drives. This OS will be used in jPhones, jPads, jMacs, and by jTV.\*

Dr. Levine, this filesystem has all the bells and whistles for assignment 3: permissions, subdirectories, and go language implementation (+ personal additions: striping and corruption detection)!

``bash
jfs example ... # TODO
``

## Getting Started

### C
TODO

### Go
TODO

## Functionality

TODO: give a CLI example of each of these
 - permissions
 - inherited permissions
 - TODO

## Under the hood

Paths use UNIX style foreward-slash `/` directory addressing. *Directories* are *inodes* that list absolute paths to other inodes using '//' as a separator. (The root is a directory.) Symbolic links only list one absolute path in their contents. Files can also be inodes. A single inode is stored across linked blocks (NULL terminal link). The metadata and filedata of an inode are both agnostically stored in *attributes*. The *attribute table* lists key names, value bit length, and the default attribute value for all attributes. A value length limit of 0 means the attribute has an unknown (>=1) bit length terminated by 0 (such as a C-string). Both attribute keys and values can spill across multiple blocks. This means the file name can have arbitrary length :smile:. Except for `fiename` and  `filesize`, explicit attribute declaration is not required, and attribute values (such as permissions) are inherited from their containing directory of drive. `nobody` is the immutable user that owns the root. See "attribute table" below. The `filedata` (not same as block's `data` section) contains the actual file contents and is positioned after all attribute declarations.

All blocks are structured as `[ valid (1) | data  (DATA_LEN) | parity (N_PARITY) ]`. The parity is only computed from the data section. The term *disk* refers to all the bits of a disk fille while *volume* refers to the data contained on a (or multiple) disks. A volume is hosted on RAIDs which are composed  of optionally 1) mirrored, 2) chained, and/or 3) striped disks with operations performed in that order. Specifying that 60 disks compose 4 striped chains with a 3-way mirror would 1) designate disks 0-19, 20-39, and 40-59 as mirror images, 2) form 4 chains in each mirror image (the first image would have disks 0-4, 5-9, 10-14, 15-19 form the chains), and finally, 3) stripe the volume data across each of those chains. RAID settings (along with future config settings) may be passed by arg in the CLI, read from the OS environment variables, or saved in `~/.jfs_defaults`. A single disk system denotes 1 single-disk-chain with no stripes or mirror. Disks are saved and loaded from Jacob Disk Format `.jdf` files.

### Volume structure
 - Volume control block (VCB)
   - jfs format version (major): 64
   - jfs format version (minor): 64
   - volume name: ?
   - datetime last formatted: 64
   - total space: LOC_POW
   - max possible usable space: LOC_POW
   - free usable space: LOC_POW
   - num reserved sections: 32
   - reserved blocks: ? *
       - start: LOC_POW
       - end: LOC_POW
   - num users: NUM_USR_POW
   - users:
      - name: UNAME_MXLN * C_W
   - num files: LOC_POW
   - num directories: LOC_POW
   - root block start: LOC_POW
   - next free block: LOC_POW
 - inodes: N *
   - next block (or 0 if just end): LOC_POW
   - num attributes: MAX_ATTRS
   - content: ?
      - attributes: ? *
         - attr key: MAX_ATTRS_POW
         - attr_value: ? (specified by attribute or terminated by 0x00)
      - filedata: ? (determined by `file_size`)   

### Constants

 - `BLOCK_SIZE = 256`
 - `N_PARITY = 8`
 - `DATA_LEN = BLOCK_SIZE - 1 - N_PARITY`
 - `C_W = 1` # later may use 4 byte width characters
 - `MAJOR_VERSION = 0`
 - `MINOR_VERSION = 0`
 - `LOC_POW = 40`
 - `NUM_USR_POW = 8`
 - `UNAME_MXLN = 40`
 - `MAX_ATTRS_POW = 24`

### Attribute table

| Key | maxlen | inherited? | description |
| --- | ------ | ---------- | ----------- |
| filename | 0 | `false` | just this file. not full path. |
| owner | NUM_USR_POW | `false` | `nobody` owns the root |
| file_size | LOC_POW | `false` | only tabulates size of `filecontents` (not counting attributes) for inodes that store files (not directories or symlinks) |
| disk_space | LOC_POW | `false` | tabulates in increments of `BLOCK_SIZE` |
| user_permissions | 3 bits | `true` | root has `rwx` |
| everybody_permissions | 3 bits | `true` | root has `rwx` |
| last_read_dt | 64 | `false` | `size_t` |
| last_write_dt |  64 | `false` | `size_t` |
| last_exec_dt |  64 | `false` | `size_t` |

### Configuration variables

These variables may be passed as process arguments, inherited from the OS environment, or stored in `~/.jfs_config` (ordered in decreasing precedence). The `disk` arg can be used repeatedly in the CLI instead of comma separated `disks`. In that case, `disk` values are used to temporarily update the `disks` variable.

| name | type | initial config | extra |
| ---- | ---- | -------------- | ----- |
| `disks` | `string` | `` | comma separated (no space) filepaths (in the host OS) to each of the disk `.jdf` files. Example: `disk0.jdf,/tmp/mydisk,local_dir/disk2.bin` |
| `num_mirrors` | `int` | `0` | 0 means no  mirrors |
| `num_chains` | `int` | `1` |  |
| `striped` | `true\|false` | `false` |  |
| `user` | `string` | `nobody` |  |


\* References to a jOS, jShell, jScript, jVM, jPhone, jPad, jPod, jMac, ... are presented amusingly and do not imply any real intention to build a larger computer ecosystem.