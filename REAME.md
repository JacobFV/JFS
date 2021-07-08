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


## API
jFS can be tested on the command line, and directly used by your operating system with syscalls. Process arguments must have keyword identifiers (`KEY=VALUE`). Unknown argument keys are silently ignored. Keys are *not* case sensitive.

### `jfs createfs NUM_BLOCKS= BLOCK_SIZE=256 VOLUME_NAME=disk.jfs NUM_DISKS=1 MIRRORS=1 CHAINS=1 STRIPED=false USER=nobody`

creates a filesystem with NUM_BLOCKS of size BLOCK_SIZE.

### `jfs format DISKS= DISK=... VOLUME_NAME= MIRRORS=1 CHAINS=1 STRIPED=false`

Formats a disk or raid. Aliases: `formatfs`.

### `jfs combine FNAME= DISKS= DISK=... MIRRORS=1 CHAINS=1 STRIPED=false

Saves the disk(s) to a single disk file named FNAME. Aliases: `savefs`.

### `jfs install DISKS= DISK=... MIRRORS=1 CHAINS=1 STRIPED=false`

use an existing disk image(s) and updates the config to remember. Aliases: `openfs`

### `jfs list START=/ RECURSIVE=false DISKS= DISK=... MIRRORS=1 CHAINS=1 STRIPED=false USER=`

list files (and other meta-information) in START. Can list recursively down the filetree, but not across symlinks.

### `jfs remove PATH= DISKS= DISK=... MIRRORS=1 CHAINS=1 STRIPED=false USER=`

Frees the space used by an inode at PATH. Aliases: `delete`.

### `jfs rename OLDPATH= NEWPATH= DISKS= DISK=... MIRRORS=1 CHAINS=1 STRIPED=false USER=`

Renames / moves an inode. Aliases: `move`.

### `jfs put EXTERNAL_FILEPATH= INTERNAL_FILEPATH= DISKS= DISK=... MIRRORS=1 CHAINS=1 STRIPED=false USER=`

Put (store) Host OS file into the disk. Does not delete external file from host OS filesystem. Aliases: `putexternalfile`

### `jfs get INTERNAL_FILEPATH= EXTERNAL_FILEPATH= DISKS= DISK=... MIRRORS=1 CHAINS=1 STRIPED=false USER=`

Get disk file, copy from “disk” to host OS file system. Does not unlink internal file from jfs. Aliases: `getexternalfile`

### `jfs user PATH= USER= DISKS= DISK=... MIRRORS=1 CHAINS=1 STRIPED=false`

Assigns the file at PATH to USER. Aliases `set_user`.

### `jfs link EXISTING_PATH= NEW_PATH= DISKS= DISK=... MIRRORS=1 CHAINS=1 STRIPED=false USER=`

creates a hard link with path NEW_PATH to an inode at EXISTING_PATH.

### `jfs unlink PATH= DISKS= DISK=... MIRRORS=1 CHAINS=1 STRIPED=false USER=`

unlinks an inode at PATH.

### `jfs chmod USER_PERMISSIONS= ALL_PERMISSIONS= PATH= DISKS= DISK=... MIRRORS=1 CHAINS=1 STRIPED=false USER=`

Change permissions for a file

### `jfs detect PATH= RECURSIVE=false TRYFIX=false DISKS= DISK=... MIRRORS=1 CHAINS=1 STRIPED=false USER=`



other API's
diskinfo: lists meta in the VCB.
meta: lists all metadata on a file

### Algorithm

1. get commmand
2. get config vars
    1. get all config vars from process arguments
        * `disk` appends to `disks`
    2. maybe add config vars from OS environment
    3. maybe add config vars from `~/.jfs_defaults`
3. run the selected operation with all known variables
    1. operations EXIT(SUCCESS|FAILURE)

## Under the hood

Paths use UNIX style foreward-slash `/` directory addressing. *Directories* are *inodes* that list absolute paths to other inodes using '//' as a separator. (The root is a directory.) Symbolic links only list one absolute path in their contents. Files can also be inodes. A single inode is stored across linked blocks (NULL terminal link). The metadata and filedata of an inode are both agnostically stored in *attributes*. The *attribute table* lists key names, value bit length, and the default attribute value for all attributes. A value length limit of 0 means the attribute has an unknown (>=0) bit length terminated by 0 (such as a C-string). Both attribute keys and values can spill across multiple blocks. This means the file name can have arbitrary length 😊. Except for `fiename` and  `filesize`, explicit attribute declaration is not required, and attribute values (such as permissions) are inherited from their containing directory of drive. `nobody` is the immutable user that owns the root. See "attribute table" below. The `filedata` (not same as block's `data` section) contains the actual file contents and is positioned after all attribute declarations.

All blocks are structured as `[ valid (1b) | data  (DATA_LEN) | parity (N_PARITY) ]`. The parity is only computed from the data section. The term *disk* refers to all the bits of a Jacob Disk Format `.jdf` fille while *volume* refers to the data contained on a (or multiple) disks. A volume is hosted on RAIDs which are composed of optionally 1) mirrored, 2) chained, and/or 3) striped disks with operations performed in that order. Specifying `disks=disk0,disk1,disk2,you_have_to_type_it_all,disk58,disk59 --chains=4, --striped=true, --mirrors=3` would 1) designate disks 0-19, 20-39, and 40-59 as mirror images, 2) form 4 chains in each mirror image (the first image would have disks 0-4, 5-9, 10-14, 15-19 form the chains), and finally, 3) stripe the volume data across each of those chains. RAID settings (along with future config settings) may be passed by arg in the CLI, read from the OS environment variables, or saved in `~/.jfs_defaults`. A single disk system denotes 1 single-disk-chain with no stripes or mirror. 

### Volume structure
 - Volume control block (VCB)
   - block size: 64B
   - jfs format version (major): 64B
   - jfs format version (minor): 64B
   - volume name: ?
   - datetime last formatted: 64B
   - total space: LOC_POW
   - max possible usable space: LOC_POW
   - free usable space: LOC_POW
   - num reserved sections: 2B
   - reserved sections: ? *
       - start: LOC_POW
       - end: LOC_POW
   - num users: NUM_USR_POW
   - users:
      - name: ?
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

| Key | maxlen | inherited? | extra |
| --- | ------ | ---------- | ----------- |
| filename | 0 | `false` | just this file. not full path. |
| owner | NUM_USR_POW | `false` | `nobody` owns the root |
| file_size | LOC_POW | `false` | only tabulates size of `filecontents` (not counting attributes) for inodes that store files (not directories or symlinks) |
| disk_space | LOC_POW | `false` | tabulates in increments of `BLOCK_SIZE` |
| user_permissions | 3b | `true` | root has `rwx` |
| everybody_permissions | 3b | `true` | root has `rwx` |
| last_read_dt | 64b | `false` | `size_t` |
| last_write_dt |  64b | `false` | `size_t` |
| last_exec_dt |  64b | `false` | `size_t` |

### Configuration variables

These variables may be passed as process arguments, inherited from the OS environment, stored in `~/.jfs_config`, or default (ordered in decreasing precedence). The `disk` arg can be used repeatedly in the CLI instead of comma separated `disks`. In that case, `disk` values are used to temporarily update the `disks` variable.

| name | type | default | extra |
| ---- | ---- | -------------- | ----- |
| `disks` | `string` | (none) | comma separated (no space) filepaths (in the host OS) to each of the disk `.jdf` files. Example: `disk0.jdf,/tmp/mydisk,local_dir/disk2.bin` |
| `mirrors` | `int` | `0` | 0 means no  mirrors |
| `chains` | `int` | `1` |  |
| `striped` | `true\|false` | `false` |  |
| `user` | `string` | `nobody` |  |


\* References to a jOS, jShell, jScript, jVM, jPhone, jPad, jPod, jMac, ... are presented amusingly and do not imply any real intention to build a larger computer ecosystem.