# jFS

Jacob's file system (jFS) is a file system library, CLI, and shell suite. It currently requires an OS-provided filesystem substrate, but a future operating system, jOS, will directly utilize jFS to store data on high throughput jDrives. This OS will be used to run jPhones, jPads, jMacs, and (of course) JavaScript.\*

**If you are a student grader please foreward this paragraph to Dr. Levine**
Dr. Levine, this filesystem has all the bells and whistles for assignment 3: permissions, subdirectories, and go language implementation (+ personal additions: RAID mirrors, chains, and striping, corruption detection and correction, a static library, and operating system CLI intergration)!

``bash
jfs example ... # TODO
``

## Getting Started

I recommend installation options by their degree of awesomeness (# of 😎's)

Please run all the following scripts from this project's root directory. These were all tested on Ubuntu 20.04.

### Full System Installation  😎😎😎😎😎

```bash
sudo dev/install.sh --all
```

### Go Shell  😎😎😎😎

```bash
# install
sudo dev/install.sh --shell_go
jfs_shell_go

# just build and run
dev/compile_shell_go.sh
./jfs_shell_go
```

### C Shell  😎😎😎

```bash
# install
sudo dev/install.sh --shell_c
jfs_shell_c

# just build and run
dev/compile_shell_c.sh
./jfs_shell_c
```

### CLI  😎😎

```bash
# install
sudo dev/install.sh --cli

# just build and run
dev/compile_cli.sh
./jfs
```

### Just library  😎

```bash
# compile only:
dev/compile_lib.sh
```

## Examples

### Shell

```
TODO shell session
```

### CLI

```bash

$ jfs create  # TODO CLI examples

$ jfs 

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

### Library

```go
// TODO using go to call library functions
```

```c
// TODO using C to library functions
```

## Details

### Library

The jfs library provides core functionality for the shell, CLI, and whatever awsome application you have in mind. Although core library functions explicitly return `error_t` codes, they may mutate arguments. (and some utility functions actually do return structs). Programmers linking jfs will want to include `lib/jfs.c` and may also want to make language-specific translations of the definitions in `lib/def.h`.

```c
// lib/jfs.h

// TODO: put `jfs.h` here
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
    (which allows entire directories to be moved).
    
    Aliases: `move`.


jfs put_external_file EXTERNAL_FILEPATH= INTERNAL_FILEPATH= DISKS= DISK=... MIRRORS=0 CHAINS=1 STRIPED=false USER=

    Puts (stores) a single file from host OS into the jfs disk.
    Does not delete external file from host OS filesystem. 
    
    Aliases: `put`

I stopped here. Continue by copying and specifying API's from jfs.h

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

All blocks are structured as `[ xxxx xxx(valid=0/1) | data  (DATA_LEN) | (left padding if any) parity (N_PARITY) ]`. The parity is only computed from the data section. The term *disk* refers to all the bits of a Jacob Disk Format `.jdf` fille while *volume* refers to the data contained on a (or multiple) disks. A volume is hosted on RAIDs which are composed of optionally 1) mirrored, 2) chained, and/or 3) striped disks with operations performed in that order. Specifying `disks=disk0,disk1,disk2,you_have_to_type_it_all,disk58,disk59 --chains=4, --striped=true, --mirrors=3` would 1) designate disks 0-19, 20-39, and 40-59 as mirror images, 2) form 4 chains in each mirror image (the first image would have disks 0-4, 5-9, 10-14, 15-19 form the chains), and finally, 3) stripe the volume data across each of those chains. RAID settings (along with future config settings) may be passed by arg in the CLI, read from the OS environment variables, or saved in `~/.jfs_defaults`. A single disk system denotes 1 single-disk-chain with no stripes or mirror. 

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

These variables are used by the CLI. They may be passed as process / command line arguments, inherited from the OS environment, stored in `~/.jfs_config`, or default (ordered in decreasing precedence). The `disk` arg can be used repeatedly to append comma separate paths to `disks`.

| name | type | default | extra |
| ---- | ---- | -------------- | ----- |
| `disks` | `string` | (none) | comma separated (no space) filepaths (in the host OS) to each of the disk `.jdf` files. Example: `disk0.jdf,/tmp/mydisk,local_dir/disk2.bin` |
| `mirrors` | `int` | `0` | 0 means no  mirrors |
| `chains` | `int` | `1` |  |
| `striped` | `bool` | `false` |  |
| `user` | `string` | `nobody` |  |


\* References to a jOS, jShell, jScript, jVM, jPhone, jPad, jPod, jMac, ... are presented amusingly and do not imply any real intention to build a larger computer ecosystem.