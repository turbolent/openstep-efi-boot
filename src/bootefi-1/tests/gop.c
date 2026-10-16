#include <assert.h>
#include "openstep_gop.h"

int main(void)
{
    OPENSTEP_GOP_INFO info = {0};
    boot_vbe_mode mode;
    info.HorizontalResolution = 1024;
    info.VerticalResolution = 768;
    info.PixelsPerScanLine = 1088; /* padding must survive conversion */
    info.PixelFormat = 1;
    assert(openstep_gop_record(&info, 0x80000000, 16*1024*1024, &mode));
    assert(mode.xResolution == 1024 && mode.yResolution == 768);
    assert(mode.bytesPerScanline == 4352 && mode.bitsPerPixel == 32);
    assert(mode.redFieldPosition == 16 && mode.greenFieldPosition == 8);
    assert(mode.blueFieldPosition == 0 && mode.redMaskSize == 8);
    assert(mode.frameBuffer == 0x80000000 && mode.modeAttributes == 0x99);
    info.PixelFormat = 0;
    assert(openstep_gop_record(&info, 0x80000000, 16*1024*1024, &mode));
    assert(mode.redFieldPosition == 0 && mode.blueFieldPosition == 16);
    info.PixelFormat = 3;
    assert(!openstep_gop_record(&info, 0x80000000, 16*1024*1024, &mode));
    info.PixelFormat = 2;
    info.PixelInformation.RedMask = 0x7c00;
    info.PixelInformation.GreenMask = 0x3e0;
    info.PixelInformation.BlueMask = 0x1f;
    assert(openstep_gop_record(&info, 0x80000000, 16*1024*1024, &mode));
    assert(mode.bitsPerPixel == 16 && mode.redMaskSize == 5);
    info.PixelInformation.RedMask = 0xf800;
    info.PixelInformation.GreenMask = 0x7e0;
    assert(!openstep_gop_record(&info, 0x80000000, 16*1024*1024, &mode));
    info.PixelInformation.RedMask = 0x1f; /* overlapping channels */
    assert(!openstep_gop_record(&info, 0x80000000, 16*1024*1024, &mode));
    info.PixelFormat = 1;
    assert(!openstep_gop_record(&info, 0x100000000ULL, 16*1024*1024, &mode));
    assert(!openstep_gop_record(&info, 0xfffff000, 16*1024*1024, &mode));
    assert(!openstep_gop_record(&info, 0x100000000ULL - 4352*1024,
                                16*1024*1024, &mode));
    assert(!openstep_gop_record(&info, 0x80000000, 1000, &mode));
    info.PixelsPerScanLine = 16384; /* 16-bit rowBytes would overflow */
    assert(!openstep_gop_record(&info, 0x80000000, 0xffffffff, &mode));
    info.PixelsPerScanLine = 1023;
    assert(!openstep_gop_record(&info, 0x80000000, 16*1024*1024, &mode));
    info.PixelsPerScanLine = 1088;
    info.VerticalResolution = 1200; /* stock driver under-maps portrait */
    assert(!openstep_gop_record(&info, 0x80000000, 16*1024*1024, &mode));
    /* MacBook7,1 probe photographs: 1280 visible pixels, 2048-pixel pitch. */
    info.HorizontalResolution = 1280;
    info.VerticalResolution = 800;
    info.PixelsPerScanLine = 2048;
    info.PixelFormat = 1;
    assert(openstep_gop_record(&info, 0xc0010000, 0x640000, &mode));
    assert(mode.bytesPerScanline == 8192 && mode.frameBuffer == 0xc0010000);
    assert(mode.xResolution == 1280 && mode.yResolution == 800 && mode.bitsPerPixel == 32);
    assert(mode.redFieldPosition == 16 && mode.greenFieldPosition == 8 && mode.blueFieldPosition == 0);
    return 0;
}
