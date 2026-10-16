#include "loader_diagnostics.h"
/* Reserve boot-2's fixed workspace while native firmware can still return
 * to its caller. Never use an occupied range after an allocation failure. */
#include "efi_reservations.h"
#include "efi_workspace.h"
#include "efi_map.h"

static const struct { UINT64 start, end; } ranges[] = {
    { OPENSTEP_LOW_START, OPENSTEP_LOW_END },
    { OPENSTEP_HIGH_START, OPENSTEP_HIGH_END },
};
static unsigned acquired;

static void say(CHAR16 *s) { gST->ConOut->OutputString(gST->ConOut, s); }
static void hex(UINT64 value, unsigned digits)
{
    CHAR16 text[17];
    unsigned i;
    for (i = 0; i < digits; i++) {
        unsigned digit = (unsigned)(value >> ((digits-i-1)*4)) & 15;
        text[i] = (CHAR16)(digit < 10 ? '0'+digit : 'A'+digit-10);
    }
    text[digits] = 0;
    say(text);
}
static void extent(UINT64 start, UINT64 end)
{
    say(L"["); hex(start, 16); say(L", "); hex(end, 16); say(L")");
}

static EFI_STATUS read_map(void **out, UINTN *map_size, UINTN *map_stride)
{
    OPENSTEP_EFI_MAP map;
    EFI_STATUS status = efi_map_acquire(&map);
    if (status == EFI_SUCCESS) {
        *out = map.data; *map_size = map.size; *map_stride = map.stride;
    }
    return status;
}

static void report_map(const void *map, UINTN size, UINTN stride, UINT64 start, UINT64 end)
{
    UINTN offset;
    say(L"Overlapping EFI descriptors (end exclusive):\r\n");
    say(L"Types: 01/02=loader 03/04=boot 05/06=runtime 07=free 0A=ACPI NVS\r\n");
    for (offset = 0; offset < size; offset += stride) {
        const EFI_MEMORY_DESCRIPTOR *d = (const void *)((const UINT8 *)map + offset);
        UINT64 limit = d->PhysicalStart + d->NumberOfPages*4096;
        if (d->PhysicalStart >= end || limit <= start) continue;
        say(L"  type "); hex(d->Type, 8); say(L" ");
        extent(d->PhysicalStart, limit); say(L"\r\n");
    }
}

EFI_STATUS efi_reserve_ranges(void)
{
    unsigned i;
    void *map = 0;
    UINTN size, stride;
    EFI_STATUS status = read_map(&map, &size, &stride);
    if (EFI_ERROR(status)) {
        say(L"[M] Cannot preflight memory map: 0x"); hex(status, 16); say(L"\r\n");
        return status;
    }
    /* Check BOTH entire ranges before any AllocateAddress call. Older
     * firmware may retype the free prefix before failing at an NVS page. */
    for (i = 0; i < sizeof(ranges)/sizeof(ranges[0]); i++) {
        if (efi_memory_end(map, size, stride, ranges[i].start, ranges[i].end, 1)
            != ranges[i].end) {
            report_map(map, size, stride, ranges[i].start, ranges[i].end);
            say(L"[M] Unavailable before allocation ");
            extent(ranges[i].start, ranges[i].end); say(L"\r\n");
            gBS->FreePool(map);
            return EFI_NOT_FOUND;
        }
    }
    gBS->FreePool(map);
    if (OPENSTEP_DIAGNOSTICS) say(L"[M] Both workspace ranges are free; reserving.\r\n");
    for (i = 0; i < sizeof(ranges)/sizeof(ranges[0]); i++) {
        UINT64 address = ranges[i].start;
        UINTN pages = (UINTN)((ranges[i].end - address)/4096);
        if (OPENSTEP_DIAGNOSTICS) {
            say(L"[M] Reserve "); extent(address, ranges[i].end);
            say(L" pages=0x"); hex(pages, 8); say(L"\r\n");
        }
        /* sarld executes inside the first range on both EFI architectures. */
        status = gBS->AllocatePages(AllocateAddress, EfiLoaderCode, pages, &address);
        if (EFI_ERROR(status)) {
            EFI_STATUS map_status = read_map(&map, &size, &stride);
            if (!EFI_ERROR(map_status)) {
                report_map(map, size, stride, ranges[i].start, ranges[i].end);
                gBS->FreePool(map);
            }
            say(L"[M] FAILED "); extent(ranges[i].start, ranges[i].end);
            say(L"\r\nEFI AllocatePages status: 0x"); hex(status, 16);
            say(L"\r\nNo kernel loaded. Firmware services remain active.\r\n");
            return status;
        }
        acquired++;
        if (OPENSTEP_DIAGNOSTICS) say(L"[M] Reserved.\r\n");
    }
    return EFI_SUCCESS;
}

void efi_release_ranges(void)
{
    while (acquired) {
        unsigned i = --acquired;
        gBS->FreePages(ranges[i].start, (UINTN)((ranges[i].end-ranges[i].start)/4096));
    }
}

void efi_reservation_wait(void)
{
    unsigned tick;
    say(L"Photograph this report. Esc = return; timeout = 60 seconds.\r\n");
    for (tick = 0; tick < 6000; tick++) {
        EFI_INPUT_KEY key;
        if (gST->ConIn &&
            !EFI_ERROR(gST->ConIn->ReadKeyStroke(gST->ConIn, &key)) &&
            (key.ScanCode == 0x17 || key.UnicodeChar == 27)) break;
        gBS->Stall(10000);
    }
}
