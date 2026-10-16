/* Native firmware-only diagnostic for a Mac that retains its boot picker.
 * ConsoleControl ABI: OpenCorePkg Include/Intel/Protocol/ConsoleControl.h.
 * No CPU-mode changes, fixed-address allocations, disk I/O or kernel entry.
 */
#include "firmware_graphics.h"

EFI_SYSTEM_TABLE *gST;
EFI_BOOT_SERVICES *gBS;
EFI_HANDLE gImageHandle;
EFI_STATUS efi_main(EFI_HANDLE, EFI_SYSTEM_TABLE *);
/* Force a real PE relocation so firmware can place this probe anywhere. */
EFI_STATUS (*volatile probe_entry)(EFI_HANDLE, EFI_SYSTEM_TABLE *) = efi_main;

typedef struct console_control CONSOLE_CONTROL;
struct console_control {
    EFI_STATUS (*GetMode)(CONSOLE_CONTROL *, UINT32 *, BOOLEAN *, BOOLEAN *);
    EFI_STATUS (*SetMode)(CONSOLE_CONTROL *, UINT32);
    void *LockStdIn;
};
static EFI_GUID console_guid =
    {0xf42f7782,0x012e,0x4c12,{0x99,0x56,0x49,0xf9,0x43,0x04,0xf7,0x21}};
static CONSOLE_CONTROL *console;
static UINT32 original_mode;
static int restore_console;

#ifndef EFI_PROBE_HOST_TEST
void *memcpy(void *dest, const void *src, UINTN n)
{ UINT8 *d = dest; const UINT8 *s = src; while (n--) *d++ = *s++; return dest; }
void *memset(void *dest, int c, UINTN n)
{ UINT8 *d = dest; while (n--) *d++ = (UINT8)c; return dest; }
#endif
static void say(CHAR16 *text) { gST->ConOut->OutputString(gST->ConOut, text); }
static void hex(UINT64 value)
{
    CHAR16 text[19];
    unsigned i;
    text[0] = '0'; text[1] = 'x'; text[18] = 0;
    for (i = 0; i < 16; i++) {
        unsigned digit = (unsigned)(value >> ((15-i)*4)) & 15;
        text[i+2] = (CHAR16)(digit < 10 ? '0'+digit : 'A'+digit-10);
    }
    say(text);
}
static void field(CHAR16 *name, UINT64 value)
{ say(name); hex(value); say(L"\r\n"); }

static EFI_STATUS text_console(void)
{
    EFI_HANDLE handles[16];
    UINTN bytes = sizeof(handles), i;
    EFI_STATUS status;
    status = gBS->HandleProtocol(gST->ConsoleOutHandle, &console_guid, (void **)&console);
    if (EFI_ERROR(status)) {
        status = gBS->LocateHandle(ByProtocol, &console_guid, 0, &bytes, handles);
        if (EFI_ERROR(status)) return status;
        for (i = 0; i < bytes/sizeof(*handles); i++) {
            status = gBS->HandleProtocol(handles[i], &console_guid, (void **)&console);
            if (!EFI_ERROR(status)) break;
        }
    }
    if (EFI_ERROR(status) || !console) return EFI_NOT_FOUND;
    status = console->GetMode(console, &original_mode, 0, 0);
    if (EFI_ERROR(status)) return status;
    if (original_mode == 0) return EFI_SUCCESS;
    if (original_mode != 1) return EFI_UNSUPPORTED;
    status = console->SetMode(console, 0); /* EfiConsoleControlScreenText */
    restore_console = !EFI_ERROR(status);
    return status;
}

static void memory_report(void)
{
    static const UINT64 addresses[] = {0xf000, 0x100000, 0x08000000, 0x09000000};
    UINTN size = 0, stride = sizeof(EFI_MEMORY_DESCRIPTOR), key = 0, capacity, offset, i;
    UINT32 version = 0;
    UINT8 *map = 0;
    EFI_STATUS status;
    say(L"[M1] Calling GetMemoryMap sizing...\r\n");
    status = gBS->GetMemoryMap(&size, 0, &key, &stride, &version);
    if (status != EFI_BUFFER_TOO_SMALL || stride < sizeof(EFI_MEMORY_DESCRIPTOR) ||
        size > 1024*1024 || stride > 4096) {
        field(L"Unexpected map sizing status: ", status); return;
    }
    capacity = size + 32*stride;
    say(L"[M2] Allocating a temporary map buffer...\r\n");
    status = gBS->AllocatePool(EfiLoaderData, capacity, (void **)&map);
    if (EFI_ERROR(status)) { field(L"AllocatePool status: ", status); return; }
    size = capacity;
    say(L"[M3] Reading memory map...\r\n");
    status = gBS->GetMemoryMap(&size, (void *)map, &key, &stride, &version);
    field(L"GetMemoryMap status: ", status);
    if (!EFI_ERROR(status) && size <= capacity && stride >= sizeof(EFI_MEMORY_DESCRIPTOR)) {
        say(L"Types: 7=conventional/free, 1/2=loader, 3/4=boot services.\r\n");
        for (i = 0; i < sizeof(addresses)/sizeof(addresses[0]); i++) {
            say(L"Address "); hex(addresses[i]); say(L" type ");
            for (offset = 0; offset+stride <= size; offset += stride) {
                EFI_MEMORY_DESCRIPTOR *d = (void *)(map+offset);
                if (addresses[i] >= d->PhysicalStart &&
                    (addresses[i]-d->PhysicalStart)/4096 < d->NumberOfPages) {
                    hex(d->Type); break;
                }
            }
            if (offset+stride > size) say(L"not mapped");
            say(L"\r\n");
        }
    }
    gBS->FreePool(map);
    say(L"[M4] Memory query complete; no fixed ranges were reserved.\r\n");
}

static void graphics_report(void)
{
    OPENSTEP_FRAMEBUFFER fb = {0};
    EFI_STATUS status;
    say(L"[G1] Querying current GOP / UGA + Apple framebuffer...\r\n");
    status = firmware_framebuffer(0, 0, &fb);
    field(L"[G2] Query status: ", status);
    if (!EFI_ERROR(status)) {
        say(fb.source == 1 ? L"Source: GOP\r\n" : L"Source: UGA + Apple\r\n");
        field(L"Framebuffer address: ", fb.base);
        field(L"Framebuffer bytes:   ", fb.size);
        field(L"Width:               ", fb.info.HorizontalResolution);
        field(L"Height:              ", fb.info.VerticalResolution);
        field(L"Pixels per row:      ", fb.info.PixelsPerScanLine);
        field(L"Pixel format:        ", fb.info.PixelFormat);
    }
    say(L"[G3] Graphics query complete; no framebuffer was written.\r\n");
}

EFI_STATUS efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *table)
{
    EFI_STATUS status;
    UINTN idle = 0;
    gST = table; gBS = table->BootServices; gImageHandle = image;
    if (!gST->ConOut || !gST->ConIn) return EFI_UNSUPPORTED;
    say(L"EFI64 PROBE 1: entered native EFI application\r\n");
    status = text_console();
    gST->ConOut->ClearScreen(gST->ConOut);
    say(L"EFI64 PROBE 1\r\n[D0] Native entry and firmware console are working.\r\n");
    field(L"ConsoleControl status: ", status);
    say(L"Firmware vendor: ");
    if (gST->FirmwareVendor) say(gST->FirmwareVendor);
    say(L"\r\n");
    field(L"Firmware revision: ", gST->FirmwareRevision);
    field(L"EFI table revision: ", *(UINT32 *)(gST->Hdr+8));
    field(L"Probe entry address: ", (UINTN)probe_entry);
    say(L"Firmware services remain active. OPENSTEP will not start.\r\n"
          L"M = memory query; G = graphics query; Esc = return to firmware.\r\n"
          L"Returns automatically after 60 seconds without a key.\r\n");
    for (;;) {
        EFI_INPUT_KEY key;
        status = gST->ConIn->ReadKeyStroke(gST->ConIn, &key);
        if (!EFI_ERROR(status)) {
            idle = 0;
            if (key.ScanCode == 0x17 || key.UnicodeChar == 27) break;
            if (key.UnicodeChar == 'm' || key.UnicodeChar == 'M') memory_report();
            if (key.UnicodeChar == 'g' || key.UnicodeChar == 'G') graphics_report();
        } else if (status != EFIERR(6)) { /* EFI_NOT_READY */
            field(L"ReadKeyStroke failed: ", status); break;
        }
        if (++idle >= 6000) { say(L"Idle timeout.\r\n"); break; }
        gBS->Stall(10000);
    }
    say(L"[D1] Returning to firmware.\r\n");
    if (restore_console) console->SetMode(console, original_mode);
    return EFI_SUCCESS;
}
