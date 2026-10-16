/* Exercise the Apple graphics-console transition and native probe exit paths. */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "firmware_graphics.h"
extern EFI_STATUS efi_main(EFI_HANDLE, EFI_SYSTEM_TABLE *);

typedef struct console CONSOLE;
struct console {
    EFI_STATUS (*GetMode)(CONSOLE *, UINT32 *, BOOLEAN *, BOOLEAN *);
    EFI_STATUS (*SetMode)(CONSOLE *, UINT32);
    void *LockStdIn;
};
static UINT32 mode;
static int present, changes, map_calls, graphics_calls, allocations, d0, d1, stalls, key_index;
static const char *keys;
static EFI_STATUS output(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *self, CHAR16 *s)
{
    (void)self;
    if (mode) return EFI_UNSUPPORTED; /* Apple's picker can conceal text. */
    for (; *s; s++) {
        if (s[0] == '[' && s[1] == 'D' && s[2] == '0') d0++;
        if (s[0] == '[' && s[1] == 'D' && s[2] == '1') d1++;
    }
    return 0;
}
static EFI_STATUS clear(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *self)
{ (void)self; assert(mode == 0); return 0; }
static EFI_STATUS get_mode(CONSOLE *self, UINT32 *out, BOOLEAN *gop, BOOLEAN *locked)
{ (void)self; (void)gop; (void)locked; *out = mode; return 0; }
static EFI_STATUS set_mode(CONSOLE *self, UINT32 value)
{ (void)self; assert(value <= 1); changes++; mode = value; return 0; }
static CONSOLE console = {get_mode,set_mode,0};
static EFI_STATUS handle(EFI_HANDLE h, EFI_GUID *guid, void **out)
{
    (void)h;
    assert(guid->d1 == 0xf42f7782);
    if (!present) return EFI_NOT_FOUND;
    *out = &console; return 0;
}
static EFI_STATUS locate(UINT32 kind, EFI_GUID *guid, void *reg, UINTN *size, EFI_HANDLE *out)
{
    (void)kind; (void)guid; (void)reg; (void)size; (void)out;
    assert(!present); return EFI_NOT_FOUND;
}
static EFI_STATUS key(EFI_SIMPLE_TEXT_INPUT_PROTOCOL *self, EFI_INPUT_KEY *out)
{
    (void)self;
    assert(d0 == 1); /* Queries happen only after the entry screen and a key. */
    if (!keys[key_index]) return EFIERR(6);
    out->ScanCode = 0; out->UnicodeChar = (UINT16)keys[key_index++]; return 0;
}
static EFI_STATUS stall(UINTN us) { assert(us == 10000); stalls++; return 0; }
static EFI_STATUS alloc(EFI_MEMORY_TYPE type, UINTN size, void **out)
{ assert(type == EfiLoaderData); allocations++; *out = malloc(size); assert(*out); return 0; }
static EFI_STATUS release(void *p) { allocations--; free(p); return 0; }
static EFI_STATUS map(UINTN *size, EFI_MEMORY_DESCRIPTOR *out, UINTN *key,
                      UINTN *stride, UINT32 *version)
{
    map_calls++; *stride = sizeof(*out); *size = sizeof(*out); *key = 1; *version = 1;
    if (!out) return EFI_BUFFER_TOO_SMALL;
    out->Type = EfiConventionalMemory; out->PhysicalStart = 0; out->NumberOfPages = 0x100000;
    return 0;
}
EFI_STATUS firmware_framebuffer(UINT32 w, UINT32 h, OPENSTEP_FRAMEBUFFER *fb)
{
    assert(w == 0 && h == 0); graphics_calls++;
    fb->source = 2; fb->base = 0x80000000; fb->size = 1280*800*4;
    fb->info.HorizontalResolution = 1280; fb->info.VerticalResolution = 800;
    fb->info.PixelsPerScanLine = 1280; fb->info.PixelFormat = 1;
    return 0;
}
int main(int argc, char **argv)
{
    EFI_SYSTEM_TABLE table = {0};
    EFI_BOOT_SERVICES bs = {0};
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL conout = {0};
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL conin = {0};
    int which = argc > 1 ? atoi(argv[1]) : 0;
    present = which != 1; mode = which == 0 ? 1 : 0;
    keys = which == 0 ? "mg\033" : which == 1 ? "\033" : "";
    conout.OutputString = output; conout.ClearScreen = clear; conin.ReadKeyStroke = key;
    table.ConOut = &conout; table.ConIn = &conin; table.BootServices = &bs;
    table.FirmwareVendor = (CHAR16 *)L"Firmware mock";
    bs.HandleProtocol = handle; bs.LocateHandle = locate; bs.Stall = stall;
    bs.GetMemoryMap = map; bs.AllocatePool = alloc; bs.FreePool = release;
    /* AllocatePages, ExitBootServices and all disk/mode-switch interfaces
     * remain null: this probe must never use them. */
    assert(efi_main((EFI_HANDLE)1, &table) == 0);
    assert(d0 == 1 && d1 == 1 && allocations == 0);
    assert(changes == (which == 0 ? 2 : 0));
    assert(mode == (which == 0 ? 1U : 0U));
    assert(map_calls == (which == 0 ? 2 : 0));
    assert(graphics_calls == (which == 0 ? 1 : 0));
    if (which == 2) assert(stalls == 5999);
    puts("EFI probe console, key-gated queries, cleanup and exit checks passed.");
    return 0;
}
