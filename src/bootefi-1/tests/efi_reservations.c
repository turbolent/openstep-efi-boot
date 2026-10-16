#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "efi_reservations.h"
#include "efi_workspace.h"

EFI_SYSTEM_TABLE *gST;
EFI_BOOT_SERVICES *gBS;
EFI_HANDLE gImageHandle;
static int scenario, calls, freed, maps, pools, pool_frees, ticks;
static char transcript[16384];
static struct descriptor { EFI_MEMORY_DESCRIPTOR d; UINT64 padding; }
    descriptors[4], map_buffer[12];
static EFI_STATUS output(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *self, CHAR16 *text)
{
    size_t n = strlen(transcript);
    (void)self;
    while (*text) { assert(n+1 < sizeof(transcript)); transcript[n++] = (char)*text++; }
    transcript[n] = 0; return 0;
}
static EFI_STATUS allocate(EFI_ALLOCATE_TYPE kind, EFI_MEMORY_TYPE type, UINTN pages, UINT64 *address)
{
    calls++;
    assert(kind == AllocateAddress && type == EfiLoaderCode);
    assert(*address == (calls == 1 ? 0xf000 : 0x100000));
    assert(pages == (calls == 1 ? 67 : 1792));
    if (scenario == 1 || (scenario == 2 && calls == 2)) return EFI_NOT_FOUND;
    return 0;
}
static EFI_STATUS release(UINT64 address, UINTN pages)
{
    if ((scenario == 0 || scenario == 3) && !freed)
        assert(address == 0x100000 && pages == 1792);
    else assert(address == 0xf000 && pages == 67);
    freed++; return 0;
}
static EFI_STATUS memory_map(UINTN *size, EFI_MEMORY_DESCRIPTOR *map, UINTN *key,
                             UINTN *stride, UINT32 *version)
{
    (void)key; (void)version; maps++;
    *stride = scenario == 6 ? 0 : sizeof(descriptors[0]);
    if (scenario == 4) return EFI_UNSUPPORTED;
    if (scenario == 10) return EFI_SUCCESS;
    if (!map || (scenario == 3 && maps == 2)) {
        *size = sizeof(descriptors); return EFI_BUFFER_TOO_SMALL;
    }
    assert(*size >= sizeof(descriptors));
    memcpy(map, descriptors, sizeof(descriptors)); *size = sizeof(descriptors);
    return 0;
}
static EFI_STATUS pool(EFI_MEMORY_TYPE type, UINTN size, void **out)
{
    (void)type; assert(size >= sizeof(descriptors) && size <= sizeof(map_buffer)); pools++;
    if (scenario == 5) return EFI_OUT_OF_RESOURCES;
    *out = map_buffer; return 0;
}
static EFI_STATUS unpool(void *p)
{ assert(p == map_buffer); pool_frees++; return 0; }
static EFI_STATUS key(EFI_SIMPLE_TEXT_INPUT_PROTOCOL *self, EFI_INPUT_KEY *out)
{ (void)self; out->ScanCode = 0x17; out->UnicodeChar = 0; return 0; }
static EFI_STATUS stall(UINTN us)
{ assert(us == 10000); ticks++; return 0; }

int main(void)
{
    EFI_SYSTEM_TABLE table = {0};
    EFI_BOOT_SERVICES services = {0};
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL conout = {0};
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL conin = {0};
    gST = &table; gBS = &services;
    table.ConOut = &conout; table.ConIn = &conin;
    conout.OutputString = output; conin.ReadKeyStroke = key;
    services.AllocatePages = allocate; services.FreePages = release;
    services.GetMemoryMap = memory_map; services.AllocatePool = pool;
    services.FreePool = unpool; services.Stall = stall;
    for (scenario = 0; scenario < 11; scenario++) {
        EFI_STATUS result;
        /* Deliberately unsorted, padded map with the MacBook's NVS hole. */
        descriptors[0].d = (EFI_MEMORY_DESCRIPTOR){EfiACPIMemoryNVS, 0, 0x8f000, 0, 1, 0};
        descriptors[1].d = (EFI_MEMORY_DESCRIPTOR){EfiConventionalMemory, 0, 0xf000, 0, 0x80, 0};
        descriptors[2].d = (EFI_MEMORY_DESCRIPTOR){EfiConventionalMemory, 0, 0x100000, 0, 0x700, 0};
        descriptors[3].d = (EFI_MEMORY_DESCRIPTOR){EfiConventionalMemory, 0, 0x90000, 0, 0x10, 0};
        if (scenario == 7) descriptors[1].d.Type = EfiLoaderCode;
        if (scenario == 8) descriptors[2].d.Type = EfiRuntimeServicesData;
        if (scenario == 9) descriptors[1].d.NumberOfPages = 0x40;
        calls = freed = maps = pools = pool_frees = 0;
        transcript[0] = 0;
        result = efi_reserve_ranges();
        if (scenario == 0 || scenario == 3) assert(result == EFI_SUCCESS);
        else assert(EFI_ERROR(result));
        efi_release_ranges();
        assert(calls == (scenario == 0 || scenario == 2 || scenario == 3 ? 2 : scenario == 1 ? 1 : 0));
        assert(freed == (scenario == 0 || scenario == 3 ? 2 : scenario == 2 ? 1 : 0));
        if (scenario == 1 || scenario == 2) {
            assert(result == EFI_NOT_FOUND);
            assert(strstr(transcript, "[M] FAILED"));
            assert(strstr(transcript, "EFI AllocatePages status: 0x"));
            assert(strstr(transcript, "No kernel loaded"));
        }
        if (scenario >= 7 && scenario <= 9) {
            assert(result == EFI_NOT_FOUND);
            assert(strstr(transcript, "[M] Unavailable before allocation"));
        }
        if (scenario == 3) assert(maps == 3 && pools == 2);
        assert(pool_frees == pools - (scenario == 5 ? 1 : 0));
    }
    descriptors[1].d.Type = EfiLoaderCode;
    descriptors[1].d.NumberOfPages = 0x80;
    assert(efi_memory_end(descriptors, sizeof(descriptors), sizeof(descriptors[0]),
                          0xf000, 0xa0000, 0) == 0x8f000);
    assert(efi_memory_end(descriptors, sizeof(descriptors), sizeof(descriptors[0]),
                          0xf000, 0xa0000, 1) == 0xf000);
    descriptors[1].d.Type = EfiConventionalMemory;
    assert(efi_memory_end(descriptors, sizeof(descriptors), sizeof(descriptors[0]),
                          0xf000, 0xa0000, 1) == 0x8f000);
    assert(efi_load_range_valid(0x30000, 0x20000));
    assert(efi_load_range_valid(0x50000, 0x2000));
    assert(!efi_load_range_valid(0x50000, 0x2001));
    assert(!efi_load_range_valid(0x8f000, 4096));
    assert(!efi_load_range_valid(0xfffff000, 8192));
    assert(efi_load_range_valid(0x780000, 0x80000));
    efi_reservation_wait(); assert(ticks == 0);
    table.ConIn = 0;
    efi_reservation_wait(); assert(ticks == 6000);
    puts("Fixed reservations: NVS hole, foreign allocations, map retries, rollback, load bounds and return wait passed.");
    return 0;
}
