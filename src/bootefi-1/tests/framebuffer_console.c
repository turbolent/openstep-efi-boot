#include <assert.h>
#include <string.h>
#include "openstep_gop.h"

char ohlfs12[96][12];
static unsigned char buffer[16+8192*800+16];

int main(void)
{
    OPENSTEP_GOP_INFO info = {0};
    boot_vbe_mode record;
    unsigned int depth, i, y, x;
    /* Use the real Mac pitch: 1280 visible pixels in an 8192-byte row.
     * The same padded row also exercises the supported 16-bit format. */
    for (depth = 16; depth <= 32; depth += 16) {
        memset(buffer, 0xa5, sizeof(buffer));
        info.HorizontalResolution = 1280;
        info.VerticalResolution = 800;
        info.PixelsPerScanLine = 8192/(depth/8);
        info.PixelFormat = depth == 32 ? 1 : 2;
        info.PixelInformation.RedMask = 0x7c00;
        info.PixelInformation.GreenMask = 0x3e0;
        info.PixelInformation.BlueMask = 0x1f;
        assert(openstep_gop_record(&info, (UINTN)(buffer+16), 8192*800, &record));
        ohlfs12['A'-32][0] = (char)0x80;
        openstep_fb_init(&record);
        openstep_fb_putc('A');
        if (depth == 32) {
            assert(*(UINT32 *)(buffer+16+2*8192+2*4) == 0xffffff);
            assert(*(UINT32 *)(buffer+16+2*8192+4*4) == 0);
        } else {
            assert(*(UINT16 *)(buffer+16+2*8192+2*2) == 0x7fff);
            assert(*(UINT16 *)(buffer+16+2*8192+4*2) == 0);
        }
        /* Force horizontal and vertical wrap, status digits and CR/LF. */
        for (i = 0; i < 3000; i++) openstep_fb_putc('A');
        openstep_fb_putc('\r');
        openstep_fb_status("\n[H2] ", 0x80000002);
        for (i = 0; i < 16; i++) {
            assert(buffer[i] == 0xa5);
            assert(buffer[sizeof(buffer)-1-i] == 0xa5);
        }
        for (i = 16; i < 16+2*8192; i++) assert(buffer[i] == 0xa5);
        for (y = 0; y < 800; y++)
            for (x = 1280*(depth/8); x < 8192; x++)
                assert(buffer[16+y*8192+x] == 0xa5);
    }
    return 0;
}
