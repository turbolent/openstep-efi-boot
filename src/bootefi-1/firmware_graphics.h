#ifndef OPENSTEP_FIRMWARE_GRAPHICS_H
#define OPENSTEP_FIRMWARE_GRAPHICS_H
#include "efi.h"

typedef struct {
    UINT32 RedMask, GreenMask, BlueMask, ReservedMask;
} OPENSTEP_PIXEL_MASKS;
typedef struct {
    UINT32 Version, HorizontalResolution, VerticalResolution, PixelFormat;
    OPENSTEP_PIXEL_MASKS PixelInformation;
    UINT32 PixelsPerScanLine;
} OPENSTEP_GOP_INFO;
typedef struct {
    UINT32 MaxMode, Mode;
    OPENSTEP_GOP_INFO *Info;
    UINTN SizeOfInfo;
    EFI_PHYSICAL_ADDRESS FrameBufferBase;
    UINTN FrameBufferSize;
} OPENSTEP_GOP_MODE;
typedef struct openstep_gop OPENSTEP_GOP;
struct openstep_gop {
    EFI_STATUS (*QueryMode)(OPENSTEP_GOP *, UINT32, UINTN *, OPENSTEP_GOP_INFO **);
    EFI_STATUS (*SetMode)(OPENSTEP_GOP *, UINT32);
    void *Blt;
    OPENSTEP_GOP_MODE *Mode;
};

/* Fixed-width data exchanged with the 32-bit loader, never EFI pointers. */
typedef struct {
    OPENSTEP_GOP_INFO info;
    UINT32 size;
    UINT64 base;
    UINT32 source; /* 1 = GOP, 2 = UGA + Apple framebuffer information */
    UINT32 changed;
} OPENSTEP_FRAMEBUFFER;
EFI_STATUS firmware_framebuffer(UINT32 width, UINT32 height,
                                OPENSTEP_FRAMEBUFFER *out);
#endif
