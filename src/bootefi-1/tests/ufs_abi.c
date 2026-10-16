/* Compare our disk-only inode with the original OPENSTEP SDK declaration. */
#include "openstep_disk.h"
#include <stddef.h>

enum { boot_ndaddr = NDADDR, boot_niaddr = NIADDR, boot_ifmt = IFMT,
       boot_ifdir = IFDIR, boot_iflnk = IFLNK, boot_fastlink = IC_FASTLINK };
#undef i_mode
#undef i_size
#undef i_db
#undef i_ib
#undef i_symlink
#undef i_icflags
/* The vnode is in-memory state only; none of its fields enter these checks. */
struct vnode { int unused; };
#include <ufs/inode.h>

#define CHECK(x) _Static_assert(x, #x)
#define FIELD(local, sdk) \
    CHECK(offsetof(struct boot_dinode, local) == offsetof(struct icommon, sdk)); \
    CHECK(sizeof(((struct boot_dinode *)0)->local) == sizeof(((struct icommon *)0)->sdk))
CHECK(sizeof(struct boot_dinode) == sizeof(struct dinode));
CHECK(sizeof(struct boot_dinode) == 128);
CHECK(sizeof(struct boot_inode) == 136);
FIELD(mode, ic_mode);
FIELD(nlink, ic_nlink);
FIELD(uid, ic_uid);
FIELD(gid, ic_gid);
FIELD(size, ic_size);
FIELD(atime, ic_atime);
FIELD(atspare, ic_atspare);
FIELD(mtime, ic_mtime);
FIELD(mtspare, ic_mtspare);
FIELD(ctime, ic_ctime);
FIELD(ctspare, ic_ctspare);
FIELD(data.blocks.direct, ic_db);
FIELD(data.blocks.indirect, ic_ib);
FIELD(data.symlink, ic_symlink);
FIELD(flags, ic_flags);
FIELD(blocks, ic_blocks);
FIELD(generation, ic_gen);
FIELD(spare, ic_spare);
CHECK(boot_ndaddr == NDADDR && boot_niaddr == NIADDR);
CHECK(boot_ifmt == IFMT && boot_ifdir == IFDIR && boot_iflnk == IFLNK);
CHECK(boot_fastlink == IC_FASTLINK);
CHECK(offsetof(struct fs, fs_magic) == 1372);
CHECK(offsetof(struct fs, fs_fmod) == 52 * 4);
CHECK(DIRBLKSIZ == 1024);
CHECK(offsetof(struct direct, d_ino) == 0);
CHECK(offsetof(struct direct, d_reclen) == 4);
CHECK(offsetof(struct direct, d_namlen) == 6);
CHECK(sizeof(((struct direct *)0)->d_namlen) == 2);
CHECK(offsetof(struct direct, d_name) == 8);
CHECK(sizeof(struct fdisk_part) == 16);
CHECK(sizeof(struct disk_blk0) == 512);
CHECK(offsetof(struct disk_blk0, parts) == 446);
CHECK(offsetof(struct disk_blk0, signature) == 510);
