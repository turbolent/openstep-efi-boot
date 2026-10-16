#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define BIOS_LEN 9216
#include "../efi_disk.c"

EFI_BOOT_SERVICES *gBS;
EFI_HANDLE gImageHandle;
char *biosbuf;
static unsigned reads, allocations, releases;
static int fail_alloc, fail_read;
static EFI_STATUS pool(EFI_MEMORY_TYPE type, UINTN size, void **out)
{
    (void)type;
    if (fail_alloc) return EFI_OUT_OF_RESOURCES;
    *out = malloc(size); assert(*out); allocations++; return 0;
}
static EFI_STATUS unpool(void *p) { releases++; free(p); return 0; }
static EFI_STATUS read_blocks(EFI_BLOCK_IO_PROTOCOL *bio, UINT32 media,
                             UINT64 lba, UINTN size, void *buffer)
{
    UINTN i, alignment = bio->Media->IoAlign;
    unsigned char *p = buffer;
    assert(media == 42 && size % bio->Media->BlockSize == 0);
    if (alignment > 1 && ((UINTN)buffer & (alignment-1))) return EFI_INVALID_PARAMETER;
    reads++;
    if (fail_read) return EFI_NOT_FOUND;
    for (i = 0; i < size; i++) p[i] = (unsigned char)((lba*bio->Media->BlockSize+i)/512);
    /* Whole-disk NeXT label at byte 7680; no MBR partition table. */
    if (lba == 0) memset(p, 0, size);
    if (lba*bio->Media->BlockSize <= 7680 && lba*bio->Media->BlockSize+size > 7683)
        memcpy(p+(UINTN)(7680-lba*bio->Media->BlockSize), "dlV3", 4);
    return 0;
}
int main(void)
{
    EFI_BOOT_SERVICES bs = {0};
    EFI_BLOCK_IO_MEDIA media = {0};
    EFI_BLOCK_IO_PROTOCOL bio = {0};
    unsigned char destination[9218];
    unsigned alignment, block;
    gBS = &bs; bs.AllocatePool = pool; bs.FreePool = unpool;
    bio.Media = &media; bio.ReadBlocks = read_blocks; media.MediaId = 42;
    media.LastBlock = 100000; disks[0] = &bio; ndisks = 1;
    biosbuf = (char *)destination+1;
    for (block = 512; block <= 2048; block *= 4) {
        media.BlockSize = block;
        for (alignment = 0; alignment <= 8192; alignment = alignment ? alignment*2 : 1) {
            media.IoAlign = alignment;
            assert(looks_like_rhapsody(&bio));
            memset(destination, 0xcc, sizeof(destination));
            assert(ebiosread(0x80, 17, 7) == 0);
            assert(destination[0] == 0xcc && destination[3585] == 0xcc);
            for (unsigned i = 0; i < 3584; i++) assert((unsigned char)biosbuf[i] == 17+i/512);
        }
    }
    media.BlockSize = 512; media.IoAlign = 4096;
    fail_alloc = 1; assert(ebiosread(0x80, 17, 7) != 0); fail_alloc = 0;
    fail_read = 1; assert(ebiosread(0x80, 17, 7) != 0); fail_read = 0;
    media.IoAlign = 3; assert(ebiosread(0x80, 17, 7) != 0);
    assert(reads && allocations == releases);
    puts("Block I/O: labels, 512/2048-byte reads, alignment through 8 KiB and failure cleanup passed.");
    return 0;
}
