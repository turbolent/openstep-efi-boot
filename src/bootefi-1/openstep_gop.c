#include "openstep_gop.h"
#include "load.h"
#include "openstep_vbe_pitch.h"

static int active;
static int preserved;

int openstep_graphics_active(void) { return active; }

void openstep_prepare_vga(void)
{
    unsigned short id;
    /* QEMU standard VGA / Bochs DISPI. Extended mode hides the VGA
     * aperture until disabled; Cirrus and other adapters skip this path. */
    __asm__ volatile("outw %0,%1" : : "a"((unsigned short)0), "Nd"((unsigned short)0x1ce));
    __asm__ volatile("inw %1,%0" : "=a"(id) : "Nd"((unsigned short)0x1cf));
    if (id >= 0xb0c0 && id <= 0xb0c5) {
        __asm__ volatile("outw %0,%1" : : "a"((unsigned short)4), "Nd"((unsigned short)0x1ce));
        __asm__ volatile("outw %0,%1" : : "a"((unsigned short)0), "Nd"((unsigned short)0x1cf));
    }
}

static unsigned int number(const char **s)
{
    unsigned int n = 0;
    while (**s >= '0' && **s <= '9') {
        n = n * 10 + (unsigned int)(*(*s)++ - '0');
        if (n > 65535)
            return 0;
    }
    return n;
}

void openstep_init_graphics(void)
{
    OPENSTEP_FRAMEBUFFER fb;
    boot_vbe_mode record;
    EFI_STATUS status = firmware_framebuffer(0, 0, &fb);
    if (!EFI_ERROR(status) &&
        openstep_gop_record(&fb.info, fb.base, fb.size, &record)) {
        preserved = 1;
        verbose("Preserving %s %dx%dx%d stride %d framebuffer %x; no VGA programming\n",
               fb.source == 1 ? "GOP" : "UGA/Apple", record.xResolution,
               record.yResolution, record.bitsPerPixel, record.bytesPerScanline,
               record.frameBuffer);
        return;
    }
#ifdef OPENSTEP_EFI64_CORE
    printf("No usable GOP or UGA/Apple framebuffer; stopping before VGA access\n");
    for (;;) ;
#else
    if (status != EFI_NOT_FOUND) {
        printf("Firmware graphics cannot be represented by OPENSTEP; boot stopped\n");
        for (;;) ;
    }
    {
        extern void efi_gfx_init(void);
        efi_gfx_init();
    }
#endif
}

void openstep_prepare_graphics(void)
{
    char *setting = newStringForKey("EFI Graphics");
    const char *p = setting;
    unsigned int width = 0, height = 0;
    OPENSTEP_FRAMEBUFFER fb;
    boot_vbe_mode record;
    EFI_STATUS status;
    if (!setting && !preserved)
        return;
    if (setting && strcmp(setting, "native")) {
        width = number(&p);
        if (*p != 'x') goto invalid;
        ++p;
        height = number(&p);
        if (*p || width < 640 || height < 480) goto invalid;
    }
    if (setting) free(setting);
    status = firmware_framebuffer(width, height, &fb);
    /* Firmware output after SetMode can restore a text-console mode. */
    if (preserved || fb.changed) active = 1;
    if (EFI_ERROR(status) || !openstep_gop_record(&fb.info, fb.base, fb.size, &record)) {
        printf("No usable requested firmware framebuffer; boot stopped\n");
        for (;;) ;
    }
    active = 1;
    openstep_fb_init(&record);
    KERNSTRUCT_ADDR->vbeFrameBuffer = 0;
    KERNSTRUCT_ADDR->vbeCurrentMode = record;
    KERNSTRUCT_ADDR->vbeModes[0] = record;
    KERNSTRUCT_ADDR->vbeModes[1].xResolution = 0;
    KERNSTRUCT_ADDR->graphicsMode = TEXT_MODE;
    /* The firmware mode and its sole modelist entry have the same depth.
     * Correct cursor addressing before the kernel initializes DriverKit. */
    {
        int pitch = openstep_fix_vbe_pitch((unsigned char *)KERNSTRUCT_ADDR->kaddr,
                               KERNSTRUCT_ADDR->ksize, record.bitsPerPixel);
        verbose("OPENSTEP VBE pitch fix: %s\n", pitch ? "applied in RAM" :
               "unrecognized kernel; not applied");
    }
    verbose("%s -> VBE mode %d: %dx%dx%d stride %d framebuffer %x (%s firmware mode)\n",
           fb.source == 1 ? "GOP" : "UGA/Apple", record.modeNumber,
           record.xResolution, record.yResolution, record.bitsPerPixel,
           record.bytesPerScanline, record.frameBuffer, fb.changed ? "selected" : "kept");
    return;
invalid:
    printf("EFI Graphics must be native or WIDTHxHEIGHT; boot stopped\n");
    free(setting);
    for (;;) ;
}
