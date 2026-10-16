#ifndef OPENSTEP_DISK_H
#define OPENSTEP_DISK_H

#include <sys/types.h>
#include <sys/param.h>
#include <bsd/dev/disk_label.h>
#include <ufs/fs.h>
#include <ufs/fsdir.h>

/* OPENSTEP 4.2's on-disk icommon layout, without the kernel vnode.
 * Layout and constants are checked against its original ufs/inode.h.
 * The source header retains the CMU, Berkeley, Sun and NeXT notices. */
#define NDADDR 12
#define NIADDR 3
#define IFMT  0170000
#define IFDIR 0040000
#define IFLNK 0120000
#define IC_FASTLINK 0x0001

struct boot_dinode {
    unsigned short mode, nlink;
    short uid, gid;
    unsigned long long size;
    int atime, atspare, mtime, mtspare, ctime, ctspare;
    union {
        struct { int direct[NDADDR], indirect[NIADDR]; } blocks;
        char symlink[60];
    } data;
    unsigned int flags, blocks, generation, spare[4];
};

struct boot_inode {
    struct boot_dinode i_din;
    ino_t i_number;
    int i_dev;
};

/* Keep the standalone reader's field names and the SDK blksize() macro. */
#define i_mode    i_din.mode
#define i_size    i_din.size
#define i_db      i_din.data.blocks.direct
#define i_ib      i_din.data.blocks.indirect
#define i_symlink i_din.data.symlink
#define i_icflags i_din.flags
#define ino_to_fsba(fs, ino) itod(fs, ino)
#define ino_to_fsbo(fs, ino) itoo(fs, ino)
#define SBOFF BBSIZE

/* PC partition-table fields used before reading the NeXT disk label. */
#define DISK_SIGNATURE 0xaa55
#define FDISK_NPART 4
#define FDISK_NEXTNAME 0xa7
struct fdisk_part {
    unsigned char bootid, beghead, begsect, begcyl;
    unsigned char systid, endhead, endsect, endcyl;
    unsigned int relsect, numsect;
};
struct disk_blk0 {
    unsigned char bootcode[446];
    unsigned char parts[FDISK_NPART][sizeof(struct fdisk_part)];
    unsigned short signature;
};

#endif
