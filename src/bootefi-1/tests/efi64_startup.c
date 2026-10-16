/* Run the real native entry point up to injected failures before LGDT/CLI.
 * Link below 4 GiB so its bridge-address check also runs as on firmware. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "efi64_bridge.h"
#include "loader_diagnostics.h"

extern EFI_STATUS efi_main(EFI_HANDLE, EFI_SYSTEM_TABLE *);
const unsigned char core_image[256] = {
    [0x3c] = 0x40, [0x77] = 8, [0x91] = 0x10
};
static UINT64 image_memory[512], stack_memory[32770], config_memory[512];
static int which, allocations, frees, outstanding, switched, restored, messages, stalled, error_messages;
static UINT32 mode = 1;
typedef struct console CONSOLE;
struct console {
    EFI_STATUS (*GetMode)(CONSOLE *, UINT32 *, BOOLEAN *, BOOLEAN *);
    EFI_STATUS (*SetMode)(CONSOLE *, UINT32);
    void *LockStdIn;
};
static EFI_STATUS get_mode(CONSOLE *self, UINT32 *out, BOOLEAN *gop, BOOLEAN *locked)
{ (void)self; (void)gop; (void)locked; *out = mode; return 0; }
static EFI_STATUS set_mode(CONSOLE *self, UINT32 value)
{
    (void)self;
    if (!value) { assert(mode == 1); switched++; }
    else { assert(mode == 0 && stalled); restored++; }
    mode = value; return 0;
}
static CONSOLE console = {get_mode, set_mode, 0};
static EFI_STATUS handle(EFI_HANDLE h, EFI_GUID *guid, void **out)
{
    assert(guid->d1 == 0xf42f7782);
    if (which == 4 || (which == 5 && h != (EFI_HANDLE)2)) return EFI_NOT_FOUND;
    *out = &console; return 0;
}
static EFI_STATUS locate(UINT32 kind, EFI_GUID *guid, void *reg, UINTN *size, EFI_HANDLE *out)
{
    (void)guid; (void)reg;
    assert(kind == ByProtocol);
    if (which == 4) return EFI_NOT_FOUND;
    assert(which == 5 && *size >= sizeof(*out));
    *size = sizeof(*out); *out = (EFI_HANDLE)2; return 0;
}
static EFI_STATUS output(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *self, CHAR16 *text)
{
    (void)self; assert(mode == 0);
    if (text[0] == '[' && text[1] == 'B' && text[2] == '0') messages++;
    if (text[0] == 'E' && text[1] == 'F' && text[2] == 'I') error_messages++;
    return 0;
}
static EFI_STATUS alloc(EFI_ALLOCATE_TYPE kind, EFI_MEMORY_TYPE type, UINTN pages,
                        EFI_PHYSICAL_ADDRESS *address)
{
    int fail_at = which < 3 ? which+1 : which == 3 ? 0 : which < 6 ? 1 : which-2;
    assert(messages == OPENSTEP_DIAGNOSTICS && mode == 0);
    allocations++;
    if (allocations == 1) {
        assert(kind == AllocateAddress && type == EfiLoaderCode);
        assert(pages == 1 && *address == 0x08000000);
    } else if (allocations < 4) {
        assert(kind == AllocateMaxAddress && type == EfiLoaderData);
        assert(*address == 0xffffffffULL);
    } else {
        assert(kind == AllocateAddress && type == EfiLoaderCode);
        assert(*address == (allocations == 4 ? 0xf000 : 0x100000));
        assert(pages == (allocations == 4 ? 67 : 1792));
    }
    if (allocations == fail_at) return EFI_OUT_OF_RESOURCES;
    /* The entry point uses the returned address. Back it with host memory. */
    if (allocations < 4)
        *address = (UINTN)(allocations == 1 ? image_memory :
                          allocations == 2 ? stack_memory : config_memory);
    outstanding++; return 0;
}
static EFI_STATUS release(EFI_PHYSICAL_ADDRESS address, UINTN pages)
{
    if (address == (UINTN)image_memory) assert(pages == 1);
    else if (address == (UINTN)stack_memory) assert(pages == 65);
    else if (address == (UINTN)config_memory) assert(pages == 1);
    else { assert(address == 0xf000 && pages == 67 && which == 7); }
    outstanding--; frees++; return 0;
}
static EFI_STATUS stall(UINTN us)
{ assert(us == 5000000 && outstanding == 0 && mode == 0); stalled++; return 0; }
static EFI_STATUS key(EFI_SIMPLE_TEXT_INPUT_PROTOCOL *self, EFI_INPUT_KEY *out)
{ (void)self; out->ScanCode = 0x17; out->UnicodeChar = 0; return 0; }
static EFI_STATUS map(UINTN *size, EFI_MEMORY_DESCRIPTOR *out, UINTN *k, UINTN *stride, UINT32 *v)
{
    (void)k; (void)v;
    *stride = sizeof(*out); *size = sizeof(*out);
    if (!out) return EFI_BUFFER_TOO_SMALL;
    *out = (EFI_MEMORY_DESCRIPTOR){EfiConventionalMemory, 0, 0, 0, 0x800, 0};
    return 0;
}
static EFI_MEMORY_DESCRIPTOR map_buffer[9];
static EFI_STATUS pool(EFI_MEMORY_TYPE type, UINTN size, void **out)
{ (void)type; assert(size <= sizeof(map_buffer)); *out = map_buffer; return 0; }
static EFI_STATUS unpool(void *p)
{ assert(p == map_buffer); return 0; }
void bridge_gate64(void) { assert(0); }
void bridge_enter32(UINTN entry, UINTN stack, EFI64_BRIDGE *bridge, void *gdt)
{ (void)entry; (void)stack; (void)bridge; (void)gdt; assert(0); }
EFI_STATUS firmware_framebuffer(UINT32 w, UINT32 h, OPENSTEP_FRAMEBUFFER *fb)
{ (void)w; (void)h; (void)fb; assert(0); return EFI_UNSUPPORTED; }

int main(int argc, char **argv)
{
    EFI_SYSTEM_TABLE table = {0};
    EFI_BOOT_SERVICES bs = {0};
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL conout = {0};
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL conin = {0};
    EFI_STATUS status;
    which = argc > 1 ? atoi(argv[1]) : 0;
    if (which == 4) mode = 0;
    conout.OutputString = output;
    table.ConOut = &conout; table.BootServices = &bs;
    conin.ReadKeyStroke = key; table.ConIn = &conin;
    bs.GetMemoryMap = map;
    bs.AllocatePool = pool; bs.FreePool = unpool;
    table.ConsoleOutHandle = (EFI_HANDLE)1;
    table.NumberOfTableEntries = which == 3 ? 257 : 0;
    bs.HandleProtocol = handle; bs.LocateHandle = locate;
    bs.AllocatePages = alloc; bs.FreePages = release; bs.Stall = stall;
    status = efi_main((EFI_HANDLE)1, &table);
    assert(status == (which == 3 ? EFI_UNSUPPORTED : EFI_OUT_OF_RESOURCES));
    assert(allocations == (which < 3 ? which+1 : which == 3 ? 2 : which < 6 ? 1 : which-2));
    assert(frees == (which < 3 ? which : which == 3 ? 2 : which < 6 ? 0 : which-3));
    assert(outstanding == 0 && messages == OPENSTEP_DIAGNOSTICS && stalled == 1);
    assert(error_messages); /* Failures remain visible in quiet builds. */
    assert(switched == (which == 4 ? 0 : 1) && restored == switched);
    assert(mode == (which == 4 ? 0U : 1U));
    puts("EFI64 early failure returned with allocations freed and console restored.");
    return 0;
}
