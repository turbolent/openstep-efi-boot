/* Firmware mocks exercise native-mode preservation and UGA's missing stride. */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "firmware_graphics.h"

EFI_SYSTEM_TABLE *gST;
EFI_BOOT_SERVICES *gBS;
EFI_HANDLE gImageHandle;
static EFI_SYSTEM_TABLE table;
static EFI_BOOT_SERVICES services;
static OPENSTEP_GOP_INFO info = {0,1280,800,1,{0,0,0,0},2048};
static OPENSTEP_GOP_MODE mode = {2,0,&info,sizeof(info),0x80010000,2048*4*800};
static OPENSTEP_GOP gop;
static int has_gop, has_uga, has_apple, sets, uga_count = 1, bad_geometry;
static EFI_STATUS alloc(EFI_MEMORY_TYPE type, UINTN n, void **p)
{ (void)type; *p = malloc(n); return *p ? 0 : EFI_OUT_OF_RESOURCES; }
static EFI_STATUS release(void *p) { free(p); return 0; }
static EFI_STATUS query(OPENSTEP_GOP *self, UINT32 m, UINTN *n, OPENSTEP_GOP_INFO **p)
{
    (void)self;
    *n = sizeof(**p); *p = malloc(*n); **p = info;
    if (m == 1) { (*p)->HorizontalResolution = 1024; (*p)->VerticalResolution = 768; }
    return 0;
}
static EFI_STATUS set(OPENSTEP_GOP *self, UINT32 m)
{
    (void)self; assert(m == 1); sets++;
    info.HorizontalResolution = 1024; info.VerticalResolution = 768;
    info.PixelsPerScanLine = 1152; /* Actual mode differs from QueryMode pitch. */
    mode.FrameBufferBase = 0x90000000; mode.FrameBufferSize = 1152*4*768;
    return 0;
}
static EFI_STATUS uga_mode(void *self, UINT32 *w, UINT32 *h, UINT32 *d, UINT32 *r)
{ (void)self; *w = 1280; *h = 800; *d = 32; *r = 60; return 0; }
static EFI_STATUS apple_info(void *self, UINT64 *base, UINT32 *size, UINT32 *row,
                              UINT32 *w, UINT32 *h, UINT32 *depth)
{
    (void)self; *base = 0x80010000; *size = 2048*4*800; *row = 2048*4;
    *w = bad_geometry ? 1024 : 1280; *h = 800; *depth = 32; return 0;
}
static struct {
    EFI_STATUS (*GetMode)(void *, UINT32 *, UINT32 *, UINT32 *, UINT32 *);
    void *SetMode, *Blt;
} uga = {uga_mode,0,0};
static struct {
    EFI_STATUS (*GetInfo)(void *, UINT64 *, UINT32 *, UINT32 *, UINT32 *, UINT32 *, UINT32 *);
} apple = {apple_info};
static EFI_STATUS locate(UINT32 type, EFI_GUID *guid, void *reg, UINTN *n, EFI_HANDLE *p)
{
    UINTN count = 0, id = 0, i;
    (void)type; (void)reg;
    if (guid->d1 == 0x9042a9de && has_gop) { count = 1; id = 1; }
    if (guid->d1 == 0x982c298b && has_uga) { count = uga_count; id = 2; }
    if (guid->d1 == 0xe316e100 && has_apple) { count = 1; id = 3; }
    if (!count) return EFI_UNSUPPORTED;
    if (!p || *n < count*sizeof(*p)) { *n = count*sizeof(*p); return EFI_BUFFER_TOO_SMALL; }
    for (i=0;i<count;i++) p[i] = (EFI_HANDLE)id;
    *n = count*sizeof(*p); return 0;
}
static EFI_STATUS protocol(EFI_HANDLE h, EFI_GUID *guid, void **p)
{
    if ((UINTN)h == 1 && guid->d1 == 0x9042a9de && has_gop) *p = &gop;
    else if ((UINTN)h == 2 && guid->d1 == 0x982c298b && has_uga) *p = &uga;
    else if ((UINTN)h == 3 && guid->d1 == 0xe316e100 && has_apple) *p = &apple;
    else return EFI_UNSUPPORTED;
    return 0;
}
int main(void)
{
    OPENSTEP_FRAMEBUFFER fb;
    gST = &table; gBS = &services;
    services.AllocatePool = alloc; services.FreePool = release;
    services.LocateHandle = locate; services.HandleProtocol = protocol;
    table.ConsoleOutHandle = (EFI_HANDLE)1;
    gop.Mode = &mode; gop.QueryMode = query; gop.SetMode = set;
    has_gop = 1;
    assert(!firmware_framebuffer(0,0,&fb));
    assert(fb.source == 1 && fb.base == 0x80010000 && fb.info.PixelsPerScanLine == 2048);
    assert(!sets && !fb.changed);
    assert(!firmware_framebuffer(1024,768,&fb));
    assert(sets == 1 && fb.changed && fb.base == 0x90000000 && fb.info.PixelsPerScanLine == 1152);
    assert(!firmware_framebuffer(1024,768,&fb) && sets == 1 && !fb.changed);
    has_gop = 0; has_uga = 1;
    assert(EFI_ERROR(firmware_framebuffer(0,0,&fb))); /* UGA alone lacks address/stride. */
    has_apple = 1;
    assert(!firmware_framebuffer(0,0,&fb));
    assert(fb.source == 2 && fb.base == 0x80010000 && fb.info.PixelsPerScanLine == 2048);
    assert(!fb.changed && sets == 1);
    assert(EFI_ERROR(firmware_framebuffer(1024,768,&fb))); /* Preserve UGA's mode. */
    bad_geometry = 1;
    assert(EFI_ERROR(firmware_framebuffer(0,0,&fb)));
    bad_geometry = 0; uga_count = 2;
    assert(EFI_ERROR(firmware_framebuffer(0,0,&fb))); /* Ambiguous display mapping. */
    has_uga = has_apple = 0;
    assert(firmware_framebuffer(0,0,&fb) == EFI_NOT_FOUND);
    puts("GOP preservation, actual stride, UGA/Apple and rejection tests passed.");
    return 0;
}
