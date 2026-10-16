/* Byte fixtures exercise the actual reader's big-endian conversions. */
#include "openstep_disk.h"
#include "ufs_byteorder.h"
#include <stddef.h>

static void big16(unsigned char *p, unsigned int n)
{ p[0] = n >> 8; p[1] = n; }
static void big32(unsigned char *p, unsigned int n)
{ p[0] = n >> 24; p[1] = n >> 16; p[2] = n >> 8; p[3] = n; }
#define AT(object, member) ((unsigned char *)&object + offsetof(__typeof__(object), member))
#define CHECK(x) do { if (!(x)) return __LINE__; } while (0)

int main(void)
{
    struct boot_dinode node = {0}, link = {0};
    struct fs fs = {0};
    union { unsigned int align; unsigned char bytes[DIRBLKSIZ]; } dir = {0};
    struct direct *entry = (struct direct *)dir.bytes;
    unsigned int i;
    big16(AT(node, mode), 0100644);
    big16(AT(node, nlink), 2);
    big16(AT(node, uid), 501);
    big16(AT(node, gid), 20);
    big32(AT(node, size), 1);
    big32(AT(node, size) + 4, 0x12345678);
    big32(AT(node, atime), 0x12345678);
    big32(AT(node, mtime), 0x23456789);
    big32(AT(node, ctime), 0x3456789a);
    for (i = 0; i < NDADDR; i++) big32(AT(node, data.blocks.direct) + 4*i, 100+i);
    for (i = 0; i < NIADDR; i++) big32(AT(node, data.blocks.indirect) + 4*i, 200+i);
    big32(AT(node, blocks), 1234);
    big32(AT(node, generation), 5678);
    big32(AT(node, spare) + 12, 0xabcdef01);
    byte_swap_dinode_in(&node);
    CHECK(node.mode == 0100644 && node.nlink == 2 && node.uid == 501 && node.gid == 20);
    CHECK(node.size == 0x112345678ULL);
    CHECK(node.atime == 0x12345678 && node.mtime == 0x23456789 && node.ctime == 0x3456789a);
    for (i = 0; i < NDADDR; i++) CHECK(node.data.blocks.direct[i] == 100+i);
    for (i = 0; i < NIADDR; i++) CHECK(node.data.blocks.indirect[i] == 200+i);
    CHECK(node.blocks == 1234 && node.generation == 5678 && node.spare[3] == 0xabcdef01);

    big16(AT(link, mode), IFLNK | 0777);
    big32(AT(link, size) + 4, 60);
    big32(AT(link, flags), IC_FASTLINK);
    for (i = 0; i < 60; i++) link.data.symlink[i] = 'a' + i % 26;
    byte_swap_dinode_in(&link);
    CHECK(link.size == 60 && link.flags == IC_FASTLINK && link.mode == (IFLNK | 0777));
    for (i = 0; i < 60; i++) CHECK(link.data.symlink[i] == 'a' + i % 26);

    big32(AT(fs, fs_magic), FS_MAGIC);
    big32(AT(fs, fs_bsize), 8192);
    big32(AT(fs, fs_fsize), 1024);
    big32(AT(fs, fs_cgrotor), 7);
    big32(AT(fs, fs_cpc), 16);
    big16(AT(fs, fs_postbl) + (MAXCPG * NRPOS - 1)*2, 0x1234);
    fs.fs_fmod = 1;
    byte_swap_superblock(&fs);
    CHECK(fs.fs_magic == FS_MAGIC && fs.fs_bsize == 8192 && fs.fs_fsize == 1024);
    CHECK(fs.fs_cgrotor == 7 && fs.fs_cpc == 16 && fs.fs_fmod == 1);
    CHECK(fs.fs_postbl[MAXCPG-1][NRPOS-1] == 0x1234);

    big32(dir.bytes, 42);
    big16(dir.bytes + 4, DIRBLKSIZ);
    big16(dir.bytes + 6, 4);
    dir.bytes[8] = 't'; dir.bytes[9] = 'e'; dir.bytes[10] = 's'; dir.bytes[11] = 't';
    byte_swap_dir_block_in((char *)dir.bytes, sizeof(dir.bytes));
    CHECK(entry->d_ino == 42 && entry->d_reclen == DIRBLKSIZ && entry->d_namlen == 4);
    CHECK(entry->d_name[0] == 't' && entry->d_name[3] == 't');
    return 0;
}
