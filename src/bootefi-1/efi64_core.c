#include "loader_diagnostics.h"
/* 32-bit EFI adapters. The boot-2 code and sarld keep their original ABI. */
#include "efi64_bridge.h"

_Static_assert(sizeof(EFI_BLOCK_IO_MEDIA) == 32, "bridge media copy");
_Static_assert(__builtin_offsetof(EFI_BLOCK_IO_PROTOCOL, ReadBlocks) == 16,
               "bridge IA32 ReadBlocks pointer");
_Static_assert(sizeof(OPENSTEP_FRAMEBUFFER) == 56 &&
               __builtin_offsetof(OPENSTEP_FRAMEBUFFER, base) == 40,
               "framebuffer bridge ABI");

struct { UINT32 address; UINT16 selector; } __attribute__((packed)) bridge_gate;
UINT32 bridge_code32;
static EFI_SYSTEM_TABLE system_table;
static EFI_BOOT_SERVICES services;
static EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL output;
static EFI_SIMPLE_TEXT_INPUT_PROTOCOL input;

static EFI_STATUS allocate(EFI_MEMORY_TYPE type, UINTN size, void **out)
{ return bridge_call(BR_ALLOC, size, out); }
static EFI_STATUS release(void *p)
{ return bridge_call(BR_FREE, p); }
static EFI_STATUS pages(EFI_ALLOCATE_TYPE at, EFI_MEMORY_TYPE mt, UINTN n, UINT64 *p)
{ return bridge_call(BR_PAGES, at, mt, n, p); }
static EFI_STATUS memory_map(UINTN *size, EFI_MEMORY_DESCRIPTOR *map, UINTN *key,
                              UINTN *stride, UINT32 *version)
{ return bridge_call(BR_MAP, size, map, key, stride, version); }
static EFI_STATUS locate(UINT32 type, EFI_GUID *guid, void *registration,
                         UINTN *size, EFI_HANDLE *handles)
{ return bridge_call(BR_LOCATE, guid, size, handles); }
static EFI_STATUS read_blocks(EFI_BLOCK_IO_PROTOCOL *bio, UINT32 media,
                              UINT64 lba, UINTN size, void *buffer)
{ return bridge_call(BR_READ, ((EFI32_BLOCK *)bio)->handle, media,
                     (UINT32)lba, (UINT32)(lba >> 32), size, buffer); }
static EFI_STATUS protocol(EFI_HANDLE handle, EFI_GUID *guid, void **out)
{ return bridge_call(BR_PROTOCOL, handle, guid, out, read_blocks); }
static EFI_STATUS text(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *self, CHAR16 *s)
{ return bridge_call(BR_OUTPUT, s); }
static EFI_STATUS key(EFI_SIMPLE_TEXT_INPUT_PROTOCOL *self, EFI_INPUT_KEY *key)
{ return bridge_call(BR_INPUT, key); }
static EFI_STATUS stall(UINTN us)
{ return bridge_call(BR_STALL, us); }
static EFI_STATUS watchdog(UINTN timeout, UINT64 code, UINTN size, CHAR16 *data)
{ return bridge_call(BR_WATCHDOG, timeout); }
static EFI_STATUS exit_services(EFI_HANDLE image, UINTN key)
{ return bridge_call(BR_EXIT, key); }
EFI_STATUS firmware_framebuffer(UINT32 width, UINT32 height, OPENSTEP_FRAMEBUFFER *out)
{ return bridge_call(BR_GRAPHICS, width, height, out); }

void core_main(EFI64_BRIDGE *bridge)
{
    extern EFI_STATUS efi_main(EFI_HANDLE, EFI_SYSTEM_TABLE *);
    bridge_gate.address = bridge->gate;
    bridge_gate.selector = bridge->code64;
    bridge_code32 = bridge->code32;
    if (OPENSTEP_DIAGNOSTICS) bridge_call(BR_CHECKPOINT);
    if (OPENSTEP_DIAGNOSTICS) bridge_call(BR_OUTPUT, L"[B5] Returned to IA32; starting loader services.\r\n");
    services.AllocatePages = pages;
    services.AllocatePool = allocate;
    services.FreePool = release;
    services.GetMemoryMap = memory_map;
    services.LocateHandle = locate;
    services.HandleProtocol = protocol;
    services.Stall = stall;
    services.SetWatchdogTimer = watchdog;
    services.ExitBootServices = exit_services;
    output.OutputString = text;
    input.ReadKeyStroke = key;
    system_table.ConOut = &output;
    system_table.ConIn = &input;
    system_table.BootServices = &services;
    system_table.ConfigurationTable = (void *)bridge->tables;
    system_table.NumberOfTableEntries = bridge->table_count;
    efi_main((EFI_HANDLE)bridge->image, &system_table);
    for (;;) __asm__ volatile("cli; hlt");
}
