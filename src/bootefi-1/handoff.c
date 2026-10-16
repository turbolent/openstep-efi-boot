#include "loader_diagnostics.h"
#include "efi.h"
#include "load.h"	/* printf(), via libsaio.h */
#include "kernBootStruct.h"	/* TEXT_MODE */
#include "efi_memmap.h"
#include "efi_map.h"

/* boot2's GDT (src/boot-2/i386/libsaio/table.c), used verbatim: selector
 * 0x20 is flat data, 0x28 flat code.  table.c defines struct seg_desc
 * itself rather than in a shared header, so its layout is reproduced here
 * (6 entries, matching table.c's NGDTENT) purely so this translation unit
 * can take Gdt's address -- no field of it is read or written here. */
struct seg_desc {
    unsigned short  limit_15_0;
    unsigned short  base_15_0;
    unsigned char   base_23_16;
    unsigned char   bit_15_8;
    unsigned char   bit_23_16;
    unsigned char   base_31_24;
};
extern struct seg_desc Gdt[6];

extern void efi_handoff(unsigned int entry);
extern void openstep_prepare_interrupts(void);
extern void openstep_prepare_graphics(void);
extern int openstep_graphics_active(void);
extern void openstep_fb_status(const char *, unsigned int);

/* efi_vga.c: resets the VGA card to standard text mode 3 (register
 * programming + font reload + buffer clear) so the kernel's own VGA
 * console setup lands on hardware left in a known-good state, matching
 * what SeaBIOS/boot1 leave behind on the legacy path. Direct port/memory
 * I/O only -- safe to call after ExitBootServices. */
extern void efi_vga_reset_text_mode(void);
extern int efi_gfx_active(void);
extern void setMode(int mode);

struct gdt_descriptor {
    unsigned short limit;
    unsigned long  base;
} __attribute__((packed));

_Static_assert(sizeof(struct gdt_descriptor) == 6 &&
               __builtin_offsetof(struct gdt_descriptor, base) == 2,
               "GDT descriptor must match handoff.S and the x86 LGDT operand");

extern struct gdt_descriptor gdt_desc;

void efi_exit_and_start(unsigned int entry)
{
    OPENSTEP_EFI_MAP map;
    EFI_STATUS status;
    int attempted, conv, ext;

    gdt_desc.base = (unsigned long)Gdt;
    verbose("[H0] Preparing firmware framebuffer.\n");
    openstep_prepare_graphics();
    gBS->SetWatchdogTimer(0, 0, 0, 0);
    verbose("[H1] Leaving EFI boot services.\n");
    status = efi_exit_services(&map, &attempted);
    if (OPENSTEP_DIAGNOSTICS || status != EFI_SUCCESS)
        openstep_fb_status("[H2] ExitBootServices result: ", (unsigned int)status);
    if (status != EFI_SUCCESS) {
        /* After even a failed exit, firmware may be partially shut down.
         * In particular, ConOut and video-mode protocols are unsafe. */
        if (!attempted)
            printf("GetMemoryMap failed before exit: %x\n", (unsigned)status);
        for (;;) __asm__ volatile("cli; hlt");
    }
    __asm__ volatile("cli" : : : "memory");
    /* No firmware calls from here on. Both the old scalar ABI and the
     * optional E820 extension must describe this same final map. */
    efi_map_limits(&map, &conv, &ext);
    KERNSTRUCT_ADDR->convmem = conv;
    KERNSTRUCT_ADDR->extmem = ext;
    if (OPENSTEP_DIAGNOSTICS) openstep_fb_status("[H3] Final extended memory KiB: ", ext);
    KERNSTRUCT_ADDR->memMapCount = efi_to_e820(map.data, map.size, map.stride,
                                KERNSTRUCT_ADDR->memMap, BOOT_MEMMAP_MAX);
    if (!openstep_graphics_active() && !efi_gfx_active())
        efi_vga_reset_text_mode();
    openstep_prepare_interrupts();
    if (OPENSTEP_DIAGNOSTICS) openstep_fb_status("[H4] Jumping to kernel entry: ", entry);
    efi_handoff(entry);
    for (;;) __asm__ volatile("cli; hlt");
}
