#include "openstep_gop.h"

static int mask_field(UINT32 mask, unsigned char *bits, unsigned char *shift)
{
    *bits = *shift = 0;
    if (!mask)
        return 0;
    while (!(mask & 1)) {
        ++*shift;
        mask >>= 1;
    }
    while (mask & 1) {
        ++*bits;
        mask >>= 1;
    }
    return mask == 0;
}

int openstep_gop_record(const OPENSTEP_GOP_INFO *info, UINT64 base, UINT32 size,
                        boot_vbe_mode *out)
{
    boot_vbe_mode mode = {0};
    UINT32 red, green, blue, reserved, all, bytes, stride, extent;
    if (!info || !out || info->HorizontalResolution < 640 ||
        info->HorizontalResolution > 65535 || info->VerticalResolution < 480 ||
        info->VerticalResolution > info->HorizontalResolution ||
        info->PixelsPerScanLine < info->HorizontalResolution)
        return 0;
    /* The original driver maps stride * WIDTH, so reject portrait modes.
     * Bound that larger mapping below; visible pixels use stride * height. */
    if (info->PixelFormat == 0) {
        red = 0xff; green = 0xff00; blue = 0xff0000; reserved = 0xff000000;
    } else if (info->PixelFormat == 1) {
        red = 0xff0000; green = 0xff00; blue = 0xff; reserved = 0xff000000;
    } else if (info->PixelFormat == 2) {
        red = info->PixelInformation.RedMask;
        green = info->PixelInformation.GreenMask;
        blue = info->PixelInformation.BlueMask;
        reserved = info->PixelInformation.ReservedMask;
    } else {
        return 0; /* Blt-only has no framebuffer the kernel can use. */
    }
    if ((red & green) || (red & blue) || (green & blue) ||
        ((red | green | blue) & reserved) ||
        !mask_field(red, &mode.redMaskSize, &mode.redFieldPosition) ||
        !mask_field(green, &mode.greenMaskSize, &mode.greenFieldPosition) ||
        !mask_field(blue, &mode.blueMaskSize, &mode.blueFieldPosition) ||
        mode.redMaskSize != mode.greenMaskSize ||
        mode.redMaskSize != mode.blueMaskSize)
        return 0;
    all = red | green | blue | reserved;
    if (mode.redMaskSize == 8 && (all & 0xff000000))
        bytes = 4;
    else if (mode.redMaskSize == 5 && !(all & 0xffff0000))
        bytes = 2;
    else
        return 0; /* OPENSTEP accepts 555/16 and 888/32, not RGB565/24. */
    if (info->PixelsPerScanLine > 65535 / bytes)
        return 0;
    stride = info->PixelsPerScanLine * bytes;
    extent = stride * info->HorizontalResolution;
    if (!base || base > 0xffffffffULL ||
        base + extent + (base & 4095) + 4095 > 0xffffffffULL ||
        size < stride * info->VerticalResolution)
        return 0;
    mode.modeNumber = OPENSTEP_GOP_VBE_MODE;
    mode.modeAttributes = 0x99; /* supported, colour, graphics, linear */
    mode.xResolution = (unsigned short)info->HorizontalResolution;
    mode.yResolution = (unsigned short)info->VerticalResolution;
    mode.bytesPerScanline = (unsigned short)stride;
    mode.bitsPerPixel = (unsigned char)(bytes * 8);
    mode.memoryModel = 6;
    mode.frameBuffer = (unsigned long)base;
    *out = mode;
    return 1;
}
