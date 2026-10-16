#ifndef OPENSTEP_EFI_WORKSPACE_H
#define OPENSTEP_EFI_WORKSPACE_H
#include "efi.h"

#define OPENSTEP_LOW_START 0x0000f000U
/* Patch 4 sarld: __TEXT [0x30000,0x50000), __DATA [0x50000,0x52000).
 * loadmacho checks every loaded segment against these reserved extents. */
#define OPENSTEP_LOW_END   0x00052000U
#define OPENSTEP_HIGH_START 0x00100000U
#define OPENSTEP_HIGH_END   0x00800000U
/* Match the stock loader's 512 MiB scan ceiling. Patch 4 has a 1 GiB
 * kernel segment and also needs virtual space for device mappings. */
#define OPENSTEP_MEMORY_LIMIT 0x20000000U

static __inline__ int efi_load_range_valid(UINT32 start, UINT32 size)
{
    return (start >= OPENSTEP_LOW_START && start < OPENSTEP_LOW_END &&
            size <= OPENSTEP_LOW_END-start) ||
           (start >= OPENSTEP_HIGH_START && start < OPENSTEP_HIGH_END &&
            size <= OPENSTEP_HIGH_END-start);
}

/* Walk a contiguous span in an unsorted EFI map. Before allocation only
 * conventional memory is available; after ExitBootServices loader and boot
 * services pages can also be reused. Never bridge a hole or ACPI NVS page. */
static __inline__ UINT64 efi_memory_end(const void *map, UINTN size, UINTN stride,
                                      UINT64 start, UINT64 limit, int free_only)
{
    if (stride < sizeof(EFI_MEMORY_DESCRIPTOR) || size % stride) return start;
    while (start < limit) {
        UINTN offset;
        UINT64 next = start;
        for (offset = 0; offset < size; offset += stride) {
            const EFI_MEMORY_DESCRIPTOR *d =
                (const void *)((const UINT8 *)map+offset);
            UINT64 end;
            if (d->NumberOfPages > (~(UINT64)0-d->PhysicalStart)/4096) return start;
            end = d->PhysicalStart+d->NumberOfPages*4096;
            if (start < d->PhysicalStart || start >= end) continue;
            if ((d->Attribute & EFI_MEMORY_RUNTIME) ||
                (d->Type != EfiConventionalMemory &&
                 (free_only || d->Type < EfiLoaderCode || d->Type > EfiBootServicesData)))
                return start;
            next = end < limit ? end : limit;
            break;
        }
        if (next == start) break;
        start = next;
    }
    return start;
}
#endif
