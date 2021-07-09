# jFS

Jacob's file system (jFS) is a file system and jFSS is shell for terminal interface to Jacob's file system. It currently requires an OS-provided filesystem substrate, but a future operating system, jOS, will directly utilize jFS to store data on high throughput jDrives. This OS will be used to run jPhones, jPads, jMacs, and (of course) JavaScript.\*

**If you are a student grader please show this program to Dr. Levine**
Dr. Levine, this filesystem has all the bells and whistles for assignment 3: permissions, subdirectories, and go language implementation (+ personal additions: mirrors, chains, striping, corruption detection and correction, static library, and operating system CLI intergration)!

``bash
jfs example ... # TODO
``

## Getting Started

### C
TODO

### Go
Only the shell is written in go
TODO

## Functionality

TODO: give a CLI example of each of these
 - permissions
 - inherited permissions
 - TODO


### API
jFS can be tested on the command line, and directly used by your operating system with syscalls. Process arguments must have keyword identifiers (`KEY=VALUE`). Unknown argument keys are silently ignored. Keys are *not* case sensitive. 

Every CLI endpoint is handled by both a frontend function has access to process arguments to parametrize calls to the mapped backend function. Backend functions do not call frontend funnctions. They also use separate groups of utilities. Backend functions must recieve all parameters (frontend functions supply some default value) which allow them to be linked to by other programs such as the shell jFSS. The backend functions only return error codes. However they may mutate arguments.

**Main:**
1. get commmand
2. get config vars
    1. get all config vars from process arguments
        * `disk` appends to `disks`
    2. maybe add config vars from OS environment
    3. maybe add config vars from `~/.jfs_defaults`
3. format config vars: for each config var:
    A. if it is a number: convert it to an `int64_t`
    B. if it is true or false: convert it to a boolean
    C. otherwise, leave it as a string
4. run the selected operation with all known variables
5. call `EXIT(SUCCESS)`

#### Create

```bash
jfs create NUM_BLOCKS= BLOCK_SIZE=256 VOLUME_NAME=disk DISKS= DISK=... MIRRORS=0 CHAINS=1 STRIPED=false
```

Creates new disk(s) and then formats a new volume on those disks. Aliases: `createfs`.

Disks are equally sized and ceiling rounded if necesary.

```c
error_t create(int64_t num_blocks, int32_t block_size, char* volume_name, RAIDINFO desired_raid_info)
```

1. create disk files
2. use backend `format` to format new disk files

#### Format

```bash
jfs format DISKS= DISK=... VOLUME_NAME=disk.jfs BLOCK_SIZE=256 MIRRORS=0 CHAINS=1 STRIPED=false
```

Formats disk(s). Aliases: `formatfs`.

```c
error_t format(int32_t block_size, char* volume_name, RAIDINFO raid_info)
```

1. use `write_raid` to propperly write 0's to from 0 to `num_blocks*block_size`
2. write volume control block
    1. generate VCB in memory with `new_VCB`
    2. serialize VCB to bytes with `serialize_VCB`
    3. store byte-serialized VCB on raid with `write_data`

#### Combine

```bash
jfs combine NEWNAME= DISKS= DISK=... MIRRORS=1 CHAINS=1 STRIPED=false
```
Saves the disk(s) to a single disk file named NEWNAME. Aliases: `savefs`.

```c
error_t combine(char* new_name, RAIDINFO raid_info)
```
1. use backend `create` to make a new disk named `NEWNAME`
2. taking the first disk option in case of mirror discrepenncies, copy all bits raw onto to the new disk. 

#### Mount

```bash
jfs mount DISKS= DISK=... MIRRORS=0 CHAINS=1 STRIPED=false
```

Updates the config file to remember what disk configuration is being used. Aliases: `openfs`

```c
error_t mount(RAIDINFO raid_info)
```
1. update config file with disk information.

#### Unmount

```bash
jfs unmount
```

Removes disk configuration information from the config file.

```c
error_t unmount()
```
1. remove disk information from config file.

#### List

```bash
jfs list START=/ META=true CONTENTS=false RECURSIVE=false DISKS= DISK=... MIRRORS=0 CHAINS=1 STRIPED=false USER=
```

list files (and other meta-information) in the inode located at START. Can list recursively down the filetree, but not across symlinks. Aliases: `meta`, `metadata`.

```c
error_t list(char* start, int8_t meta, int8_t contents, int8_t recursive, RAIDINFO raid_info)
```
1. make sure the user has permission to read the inode at START
2. print the absolute path of inode
3. if meta is true: print the attributes of the inode
4. if contents is true and the inode has a filesize attribute: print the contents of the inode
5. if the inode at start_path is a directory and recursive is true: use `list` on all inodes in the directory.

#### Remove

```bash
jfs remove PATH= UNLINK=true DISKS= DISK=... MIRRORS=0 CHAINS=1 STRIPED=false USER=
```

Frees the space used by an inode at PATH. By default, also unlinks inode from master inode table. Aliases: `delete`.

**backend remove:**
1. make sure the user has permission to write to the inode at PATH
2. edit volume information
    - increase vcb.free_usable_space
    - increase vcb.free_blocks
3. assign all valid bits for the blocks used by the inode at PATH to false
4. if unlink, also use `unlink` to unlink the path

#### Rename

```bash
jfs rename OLDPATH= NEWPATH= FIXSYMLINKS=true DISKS= DISK=... MIRRORS=0 CHAINS=1 STRIPED=false USER=
```

Renames / moves an inode from OLDPATH to NEWPATH. Aliases: `move`.

**backend rename:**
1. make sure the user has permission to write to the inodes at OLDPATH and the containing directory of NEWPATH.
2. for each inode with have a matching prefix pattern as oldpath in the master inode table:
    1. change path
    2. if the inode is a directory, rename the paths in its dests field.
    3. if fixsymlinks is true: rename the paths of all symlinks to this inode

#### Put External File

```bash
jfs put_external_file EXTERNAL_FILEPATH= INTERNAL_FILEPATH= DISKS= DISK=... MIRRORS=0 CHAINS=1 STRIPED=false USER=
```

Put (store) Host OS file into the disk. Does not delete external file from host OS filesystem. Aliases: `put`

**backend put:**
1. make sure the user has permission to write to the inodes at INTERNAL_FILEPATH
2. get file at external_path
3. modify VCB parameters:
    - 

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

### Utils

VCB* get_VCB(DISKINFO* diskinfo);

ERR write_file
// writes across multiple blocks
// decreases VCB free space and free blocks.
// TODO: make this a special case when the util's alter the VCB

int8* read_file
// reads across multiple blocks

int8* read_data(int64 block_loc, int16 data_len, DISKSINFO* disksinfo, bool force=0);
 A. if force==1, just read data section
 B. otherwise
    1. raise error if not valid
    2. read data section and compute parity
    3. raise error if parity is not correct

ERRNO write_data(int64 block_loc, int16 data_len, int8* data, DISKSINFO* disksinfo);
// agnostic to volume information (does not update VCB free space or obey reserved sections rule)
 1. set valid
 2. write to data and compute parity
 3. write parity

consts
typedef struct {
    char** disks;
    int8 mirrors;
    int8 chains;
    bool striped;
} DISKSINFO;
typedef struct {
    ... all data listed in VCB above
    (use next higher order size_t int's in most cases)
} VCB;


## Under the hood

Paths use UNIX style foreward-slash `/` directory addressing. *Directories* are *inodes* that list absolute paths to other inodes using '//' as a separator. (The root `/` is a directory.) Symbolic links only list one absolute path in their contents. Files can also be inodes. A single inode is stored across linked blocks (NULL terminal link). However a master inode table is used to store the full paths and start block number of all files. The metadata of an inode is stored in *attributes*. The *attribute table* lists key names, value bit length, inheritance, and the default value of all attributes. A value length limit of 0 means the attribute has an unknown (>=0) bit length terminated by 0 (such as a C-string). Both attribute keys and values can spill across multiple blocks. This allows file names to have arbitrary length 😊. Files must have `filesize` declared, and directories and symlinks must have `dests` declared, but otherwise attribute declaration is optional, and attribute values (such as permissions) can be inherited from their containing directory of drive. `nobody` is the immutable user that owns the root. (See "attribute table" below for more details.) The `filedata` (not same as block's `data` section) contains the actual file contents. It is positioned after all attribute declarations, and contains `filesize` bits.

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
   - free space: LOC_POW
   - free blocks: LOC_POW
   - num files: LOC_POW
   - num directories: LOC_POW
   - total inodes: LOC_POW
   - master inode table: ? *
      - absolute path: ?
      - inode start block: LOC_POW 
   - next free block: LOC_POW
   - num reserved sections: 2B
   - reserved sections: ? *
       - start: LOC_POW
       - end: LOC_POW
   - num users: NUM_USR_POW
   - users:
      - name: ?
 - inodes: ? *
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
| owner | NUM_USR_POW | `true` | `nobody` owns the root |
| file_size | LOC_POW | `false` | only tabulates size of `filecontents` (not counting attributes) for inodes that store files (not directories or symlinks) |
| disk_space | LOC_POW | `false` | tabulates in increments of `BLOCK_SIZE` |
| user_permissions | 3b | `true` | root has `rwx` |
| everybody_permissions | 3b | `true` | root has `rwx` |
| last_read_dt | 64b | `false` | `size_t` |
| last_write_dt |  64b | `false` | `size_t` |
| last_exec_dt |  64b | `false` | `size_t` |
| dests | 0 | `false` | C-string stores `/` separated paths for directories and symlinks. |

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