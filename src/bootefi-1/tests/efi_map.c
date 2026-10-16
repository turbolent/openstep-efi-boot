#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "efi_map.h"

EFI_BOOT_SERVICES *gBS;
EFI_HANDLE gImageHandle = (void *)42;
static unsigned scenario, maps, exits, allocations, frees;
static unsigned char buffer[4096];
static UINTN allocated_size;
struct padded { EFI_MEMORY_DESCRIPTOR d; UINT64 padding; };

static EFI_STATUS pool(EFI_MEMORY_TYPE type, UINTN size, void **out)
{
    assert(!exits && type == EfiLoaderData && size <= sizeof(buffer));
    allocations++;
    if (scenario == 5) return EFI_OUT_OF_RESOURCES;
    allocated_size = size; *out = buffer; return 0;
}
static EFI_STATUS unpool(void *p)
{ assert(!exits && p == buffer); frees++; return 0; }
static EFI_STATUS memory_map(UINTN *size, EFI_MEMORY_DESCRIPTOR *data, UINTN *key,
                             UINTN *stride, UINT32 *version)
{
    struct padded d[5] = {0};
    UINTN required = sizeof(d);
    maps++; *stride = sizeof(d[0]); *version = 1; *key = maps;
    if (scenario == 4) return EFI_UNSUPPORTED;
    if (scenario == 8) *stride = 0;
    if (scenario == 6 && maps == 2) required = allocated_size+sizeof(d[0]);
    if (scenario == 7 && exits) required = allocated_size+sizeof(d[0]);
    if (!data || *size < required) { *size = required; return EFI_BUFFER_TOO_SMALL; }
    assert(*size == allocated_size); /* Retry uses capacity, not old used size. */
    d[0].d = (EFI_MEMORY_DESCRIPTOR){EfiACPIMemoryNVS, 0, 0x8f000, 0, 1, 0};
    d[1].d = (EFI_MEMORY_DESCRIPTOR){EfiLoaderCode, 0, 0xf000, 0, 0x80, 0};
    d[2].d = (EFI_MEMORY_DESCRIPTOR){EfiConventionalMemory, 0, 0x90000, 0, 0x10, 0};
    d[3].d = (EFI_MEMORY_DESCRIPTOR){EfiBootServicesData, 0, 0x100000, 0, 0xf00, 0};
    d[4].d = (EFI_MEMORY_DESCRIPTOR){EfiConventionalMemory, 0, 0x1000000, 0, 0x1000, 0};
    /* Simulate a map change during the first unsuccessful ExitBootServices. */
    if (exits) d[4].d.Type = EfiACPIMemoryNVS;
    if (scenario == 10) d[3].d.NumberOfPages = ~(UINT64)0;
    if (scenario == 11) d[4].d.Attribute = EFI_MEMORY_RUNTIME;
    if (scenario == 12) d[4].d.NumberOfPages = 0xbf000; /* RAM through 3 GiB */
    memcpy(data, d, sizeof(d)); *size = sizeof(d) - (scenario == 9 ? 1 : 0);
    return 0;
}
static EFI_STATUS leave(EFI_HANDLE image, UINTN key)
{
    assert(image == gImageHandle && key == maps);
    exits++;
    if (scenario == 3) return EFI_UNSUPPORTED;
    if (scenario == 2 || ((scenario == 1 || scenario == 7) && exits == 1))
        return EFI_INVALID_PARAMETER;
    return 0;
}
int main(void)
{
    EFI_BOOT_SERVICES bs = {0};
    gBS = &bs;
    bs.GetMemoryMap = memory_map; bs.AllocatePool = pool;
    bs.FreePool = unpool; bs.ExitBootServices = leave;
    for (scenario = 0; scenario < 13; scenario++) {
        OPENSTEP_EFI_MAP map;
        int attempted, conv, ext;
        EFI_STATUS result;
        maps = exits = allocations = frees = 0;
        result = efi_exit_services(&map, &attempted);
        if (scenario == 0 || scenario == 1 || scenario == 6 || scenario == 11 || scenario == 12) {
            assert(result == EFI_SUCCESS && attempted);
            efi_map_limits(&map, &conv, &ext);
            assert(conv == 572);
            assert(ext == (scenario == 12 ? 511*1024 :
                          scenario == 1 || scenario == 11 ? 15360 : 31744));
        } else assert(result != EFI_SUCCESS);
        if (scenario == 1 || scenario == 2) assert(exits == 2 && maps == 3);
        if (scenario == 3 || scenario == 7) assert(exits == 1);
        if (scenario >= 4 && scenario <= 10 && scenario != 6 && scenario != 7)
            assert(!attempted && !exits);
        if (scenario == 6) assert(allocations == 2 && frees == 1);
        if (attempted) assert(allocations == frees+1);
    }
    puts("Memory/exit: stale key, bounded retries, map growth, bad descriptors, NVS/runtime holes and final limits passed.");
    return 0;
}
