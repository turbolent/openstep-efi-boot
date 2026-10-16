/* malloc/free for boot-2's reused sources, and the fixed-address memory
 * reservations the loader depends on (moved here from the Task 3 spike).
 */
#include "efi.h"
#include "kernBootStruct.h"
#include "efi_map.h"

extern int efi_disk_count(void);
extern int verbose(const char *, ...);

void *malloc(int size)
{
    void *p = 0;

    if (size <= 0)
        return 0;
    if (EFI_ERROR(gBS->AllocatePool(EfiLoaderData, (UINTN)size, &p)))
        return 0;
    /* boot-2's allocator guarantees zero-filled allocations.
     * Its inode/directory caches and configuration parser rely on it. */
    {
        unsigned char *byte = (unsigned char *)p;
        int left = size;
        while (left-- > 0)
            *byte++ = 0;
    }
    return p;
}

void free(char *p)
{
    if (p)
        gBS->FreePool(p);
}

/* libsaio/misc.c's newString(), reimplemented here instead of pulling in
 * that whole translation unit -- its other two functions (sleep(),
 * turnOffFloppy()) would collide with the EFI-safe versions this loader
 * already defines in efi_console.c/efi_disk.c. drivers.c/stringTable.c
 * need this one. */
extern int strlen(const char *s);
extern char *strcpy(char *s1, const char *s2);

char *newString(char *oldString)
{
    if (oldString)
        return strcpy(malloc(strlen(oldString) + 1), oldString);
    else
        return 0;
}

/* Not called by any boot-2 source directly; clang can lower a large
 * struct/array assignment to a call to memmove() (seen from sys.c), and
 * nothing in this freestanding build provides one. */
void *memmove(void *dst, const void *src, unsigned long len)
{
    char *d = (char *)dst;
    const char *s = (const char *)src;

    if (d == s || len == 0)
        return dst;
    if (d < s) {
        while (len--)
            *d++ = *s++;
    } else {
        d += len;
        s += len;
        while (len--)
            *--d = *--s;
    }
    return dst;
}

/* Not called by any boot-2 source directly under Task 5's build, but
 * load.c's LC_SEGMENT handling and this task's own kernel-load bookkeeping
 * both call bzero(). libsa/memset.c (the usual provider) is excluded from
 * this build -- see the Task 5 report -- so, as with memmove() above,
 * supply a plain C implementation here rather than pull in the
 * non-compiling original. */
int bzero(char *b, int length)
{
    while (length-- > 0)
        *b++ = 0;
    return 0;
}

/* load.c's openfile() falls back to these for a ".rcz"-compressed kernel
 * when a plain open() fails. This loader never calls openfile() -- load_
 * kernel() in efi_main.c opens mach_kernel directly, which is never
 * compressed -- but openfile() is compiled into the same object file as
 * loadprog(), so the linker still needs these symbols resolved. Stubs
 * only; the real rcz decompressor (src/boot-2/i386/rcz) is not part of
 * this build. */
int rcz_file_size(int in_fd)
{
    return -1;
}

int rcz_decompress_file(int in_fd, unsigned char *out)
{
    return -1;
}

/* Early limits for the loader. Recompute from the successful exit map
 * in handoff.c before the kernel takes ownership of memory. */
void efi_sizemem(int *convmem_kb, int *extmem_kb)
{
    OPENSTEP_EFI_MAP map;
    EFI_STATUS status = efi_map_acquire(&map);
    if (status != EFI_SUCCESS) {
        printf("GetMemoryMap failed: %x\n", (unsigned)status);
        for (;;) ;
    }
    efi_map_limits(&map, convmem_kb, extmem_kb);
    verbose("EFI contiguous memory: conventional %d KiB, extended %d KiB\n",
           *convmem_kb, *extmem_kb);
    gBS->FreePool(map.data);
}

/* Mirrors getKernBootStruct() in src/boot-2/i386/libsaio/bootstruct.c, with
 * EFI sources where the BIOS ones are gone. */
void efi_init_bootstruct(void)
{
    int conv, ext;

    bzero((char *)kernBootStruct, sizeof(*kernBootStruct));

    efi_sizemem(&conv, &ext);
    kernBootStruct->convmem = conv;
    kernBootStruct->extmem = ext;

    /* Must be non-zero: sys.c rejects hd() opens when this is 0. */
    kernBootStruct->numIDEs = efi_disk_count();

    kernBootStruct->magicCookie = KERNBOOTMAGIC;
    kernBootStruct->configEnd = kernBootStruct->config;
    kernBootStruct->graphicsMode = TEXT_MODE;
    /* Must land above the live bootstruct contents: the kernel's
     * pmap_bootstrap alloc_cnvmem() bump-allocates the IDT page and then
     * its page directory/tables starting at first_addr0, and with this at
     * 0 it builds page tables straight over KERNBOOTSTRUCT while the
     * kernel is still reading it. Mirrors boot2's own two sites that set
     * this the same way: src/boot-2/i386/libsaio/stringTable.c:749 and
     * src/boot-2/i386/libsaio/drivers.c:759. */
    kernBootStruct->first_addr0 = (int)kernBootStruct->configEnd + 1024;

    /* The kernel's setconf() reads rootdev from here, and -v turns on the
     * verbose output later tasks check for. efi_main selects hd(0,a) before
     * reading System.config and its Kernel setting. BOOTEFI_BOOT_STRING lets the
     * Makefile override this at build time (e.g. to append -s); it defaults
     * to the same string when not overridden. */
#ifndef BOOTEFI_BOOT_STRING
#define BOOTEFI_BOOT_STRING "rootdev=hd0a -v"
#endif
    strncpy(kernBootStruct->bootString, BOOTEFI_BOOT_STRING,
            BOOT_STRING_LEN - 1);
    /* diskInfo, video, pciInfo, eisaSlotInfo and apm_config stay zeroed:
     * they are BIOS-derived and have no EFI equivalent. */
}
