/* Firmware-independent handoff diagnostics using the already validated GOP
 * or UGA record and the Apple/NeXT font already linked by efi_gfx.c. */
#include "openstep_gop.h"

extern char ohlfs12[96][12];
static boot_vbe_mode mode;
static unsigned int row, column;

static void pixel(unsigned int x, unsigned int y, unsigned int colour)
{
    volatile unsigned char *p = (volatile unsigned char *)(UINTN)mode.frameBuffer +
                                y*mode.bytesPerScanline + x*(mode.bitsPerPixel/8);
    if (mode.bitsPerPixel == 32)
        *(volatile UINT32 *)p = colour;
    else
        *(volatile UINT16 *)p = (UINT16)colour;
}

static void clear_line(void)
{
    unsigned int x, y;
    for (y = 2+row*24; y < 2+(row+1)*24; y++)
        for (x = 0; x < mode.xResolution; x++) pixel(x, y, 0);
}

void openstep_fb_init(const boot_vbe_mode *record)
{
    mode = *record;
    row = column = 0;
    clear_line();
}

void openstep_fb_putc(int c)
{
    unsigned int x, y, white;
    if (!mode.frameBuffer) return;
    if (c == '\r') { column = 0; return; }
    if (c == '\n' || column == (mode.xResolution-4)/16) {
        column = 0;
        if (++row == (mode.yResolution-4)/24) row = 0;
        clear_line();
        if (c == '\n') return;
    }
    if (c < 32 || c > 127) c = '?';
    white = ((1U << mode.redMaskSize)-1) << mode.redFieldPosition |
            ((1U << mode.greenMaskSize)-1) << mode.greenFieldPosition |
            ((1U << mode.blueMaskSize)-1) << mode.blueFieldPosition;
    for (y = 0; y < 24; y++)
        for (x = 0; x < 16; x++)
            pixel(2+column*16+x, 2+row*24+y,
                  (unsigned char)ohlfs12[c-32][y/2] & (0x80 >> (x/2)) ? white : 0);
    column++;
}

/* Direct stores only: usable after a failed or successful ExitBootServices.
 * Do not route this through printf or firmware/serial protocols. */
void openstep_fb_status(const char *label, unsigned int value)
{
    int shift;
    while (*label) openstep_fb_putc(*label++);
    for (shift = 28; shift >= 0; shift -= 4)
        openstep_fb_putc("0123456789ABCDEF"[(value >> shift) & 15]);
    openstep_fb_putc('\n');
}
