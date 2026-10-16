/* GOP describes the linear aperture directly. UGA does not: on Apple EFI,
 * pair UgaDraw.GetMode with AppleFramebufferInfo.GetInfo. Never guess a BAR
 * or assume that row stride equals visible width. Protocol declarations:
 * UEFI GOP, EFI 1.10 UGA, OpenCore Include/Apple/Protocol/AppleFramebufferInfo.h.
 */
#include "firmware_graphics.h"

static EFI_GUID gop_guid =
    {0x9042a9de,0x23dc,0x4a38,{0x96,0xfb,0x7a,0xde,0xd0,0x80,0x51,0x6a}};
static EFI_GUID uga_guid =
    {0x982c298b,0xf4fa,0x41cb,{0xb8,0x38,0x77,0xaa,0x68,0x8f,0xb8,0x39}};
static EFI_GUID apple_guid =
    {0xe316e100,0x0751,0x4c49,{0x90,0x56,0x48,0x6c,0x7e,0x47,0x29,0x03}};
typedef struct uga UGA;
struct uga {
    EFI_STATUS (*GetMode)(UGA *, UINT32 *, UINT32 *, UINT32 *, UINT32 *);
    void *SetMode, *Blt;
};
typedef struct apple_fb APPLE_FB;
struct apple_fb {
    EFI_STATUS (*GetInfo)(APPLE_FB *, UINT64 *, UINT32 *, UINT32 *,
                          UINT32 *, UINT32 *, UINT32 *);
};

static EFI_HANDLE *handles_for(EFI_GUID *guid, UINTN *count)
{
    UINTN bytes = 0;
    EFI_HANDLE *handles = 0;
    *count = 0;
    if (gBS->LocateHandle(ByProtocol, guid, 0, &bytes, 0) != EFI_BUFFER_TOO_SMALL ||
        !bytes || EFI_ERROR(gBS->AllocatePool(EfiLoaderData, bytes, (void **)&handles)))
        return 0;
    if (EFI_ERROR(gBS->LocateHandle(ByProtocol, guid, 0, &bytes, handles))) {
        gBS->FreePool(handles);
        return 0;
    }
    *count = bytes / sizeof(*handles);
    return handles;
}

static int snapshot(OPENSTEP_GOP *gop, OPENSTEP_FRAMEBUFFER *out)
{
    OPENSTEP_GOP_MODE *mode = gop->Mode;
    if (!mode || !mode->Info || mode->SizeOfInfo < sizeof(out->info) ||
        mode->Info->PixelFormat > 2 || !mode->FrameBufferBase ||
        mode->FrameBufferSize > 0xffffffffULL)
        return 0;
    out->info = *mode->Info;
    out->base = mode->FrameBufferBase;
    out->size = (UINT32)mode->FrameBufferSize;
    out->source = 1;
    return 1;
}

EFI_STATUS firmware_framebuffer(UINT32 width, UINT32 height,
                                OPENSTEP_FRAMEBUFFER *out)
{
    UINTN count, i, j;
    EFI_HANDLE *handles = handles_for(&gop_guid, &count);
    int saw_display = count != 0;
    out->source = out->changed = 0;
    /* Prefer the console's handle. Remaining handles retain firmware order. */
    for (i = 0; i <= count; i++) {
        OPENSTEP_GOP *gop = 0;
        EFI_HANDLE handle = i ? handles[i-1] : gST->ConsoleOutHandle;
        if (!handle || EFI_ERROR(gBS->HandleProtocol(handle, &gop_guid, (void **)&gop)) ||
            !gop || !gop->Mode)
            continue;
        saw_display = 1;
        if (snapshot(gop, out) && (!width ||
            (out->info.HorizontalResolution == width && out->info.VerticalResolution == height)))
            goto success;
        for (j = 0; width && j < gop->Mode->MaxMode; j++) {
            OPENSTEP_GOP_INFO *info = 0;
            UINTN size = 0;
            int match;
            if (EFI_ERROR(gop->QueryMode(gop, (UINT32)j, &size, &info)) || !info)
                continue;
            match = size >= sizeof(*info) && info->PixelFormat <= 2 &&
                info->HorizontalResolution == width && info->VerticalResolution == height;
            gBS->FreePool(info);
            if (match && !EFI_ERROR(gop->SetMode(gop, (UINT32)j))) {
                out->changed = 1;
                if (snapshot(gop, out))
                    goto success;
            }
        }
    }
    if (handles) gBS->FreePool(handles);
    handles = handles_for(&uga_guid, &count);
    if (count) saw_display = 1;
    for (i = 0; i < count; i++) {
        UGA *uga = 0;
        UINT32 w, h, depth, refresh;
        EFI_HANDLE *apple_handles;
        UINTN apple_count;
        if (EFI_ERROR(gBS->HandleProtocol(handles[i], &uga_guid, (void **)&uga)) || !uga ||
            EFI_ERROR(uga->GetMode(uga, &w, &h, &depth, &refresh)) ||
            (width && (w != width || h != height)) || depth != 32)
            continue;
        apple_handles = handles_for(&apple_guid, &apple_count);
        /* GetInfo has no display identity; reject ambiguous multi-GPU UGA. */
        if (count == 1 && apple_count == 1) {
            APPLE_FB *apple = 0;
            UINT32 row, aw, ah, ad;
            if (!EFI_ERROR(gBS->HandleProtocol(apple_handles[0], &apple_guid, (void **)&apple)) &&
                apple && !EFI_ERROR(apple->GetInfo(apple, &out->base, &out->size,
                                                  &row, &aw, &ah, &ad)) &&
                aw == w && ah == h && ad == depth && !(row & 3) && row / 4 >= w) {
                OPENSTEP_GOP_INFO info = {0};
                info.HorizontalResolution = w;
                info.VerticalResolution = h;
                info.PixelFormat = 1; /* Apple's 32-bit BGRX framebuffer */
                info.PixelsPerScanLine = row / 4;
                out->info = info;
                out->source = 2;
                gBS->FreePool(apple_handles);
                goto success;
            }
        }
        if (apple_handles) gBS->FreePool(apple_handles);
    }
    if (handles) gBS->FreePool(handles);
    out->source = 0;
    return saw_display ? EFI_UNSUPPORTED : EFI_NOT_FOUND;
success:
    if (handles) gBS->FreePool(handles);
    return EFI_SUCCESS;
}
