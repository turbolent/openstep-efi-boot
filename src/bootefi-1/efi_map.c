/* UEFI 2.10 sections 7.2.3 and 7.4.6. Reviewed against OpenCore's
 * OcGetCurrentMemoryMapAlloc and ForceExitBootServices; see docs/efi-audit.json.
 * Keep allocation capacity separate from GetMemoryMap's returned size. */
#include "efi_map.h"
#include "efi_workspace.h"

EFI_STATUS efi_map_refresh(OPENSTEP_EFI_MAP *map)
{
    UINTN offset;
    EFI_STATUS status;
    map->size = map->capacity;
    status = gBS->GetMemoryMap(&map->size, map->data, &map->key,
                               &map->stride, &map->version);
    if (status != EFI_SUCCESS) return status;
    if (!map->data || !map->size || map->size > map->capacity ||
        map->stride < sizeof(EFI_MEMORY_DESCRIPTOR) || map->stride > 4096 ||
        map->stride % 8 || map->size % map->stride) return EFI_UNSUPPORTED;
    for (offset = 0; offset < map->size; offset += map->stride) {
        const EFI_MEMORY_DESCRIPTOR *d = (const void *)((const UINT8 *)map->data+offset);
        if ((d->PhysicalStart & 4095) || !d->NumberOfPages ||
            d->NumberOfPages > (~(UINT64)0-d->PhysicalStart)/4096)
            return EFI_UNSUPPORTED;
    }
    return EFI_SUCCESS;
}

EFI_STATUS efi_map_acquire(OPENSTEP_EFI_MAP *map)
{
    EFI_STATUS status;
    unsigned attempt;
    map->data = 0; map->capacity = 0;
    map->stride = sizeof(EFI_MEMORY_DESCRIPTOR);
    status = efi_map_refresh(map);
    if (status != EFI_BUFFER_TOO_SMALL) return status;
    for (attempt = 0; attempt < 4; attempt++) {
        if (map->stride < sizeof(EFI_MEMORY_DESCRIPTOR) || map->stride > 4096 ||
            map->size > (UINTN)-1 - 8*map->stride) return EFI_UNSUPPORTED;
        map->capacity = map->size + 8*map->stride;
        status = gBS->AllocatePool(EfiLoaderData, map->capacity, (void **)&map->data);
        if (status != EFI_SUCCESS) { map->data = 0; return status; }
        status = efi_map_refresh(map);
        if (status == EFI_SUCCESS) return status;
        gBS->FreePool(map->data); map->data = 0;
        if (status != EFI_BUFFER_TOO_SMALL) break;
    }
    return status;
}

EFI_STATUS efi_exit_services(OPENSTEP_EFI_MAP *map, int *attempted)
{
    EFI_STATUS status;
    unsigned attempt;
    *attempted = 0;
    status = efi_map_acquire(map);
    if (status != EFI_SUCCESS) return status;
    for (attempt = 0; attempt < 2; attempt++) {
        *attempted = 1;
        status = gBS->ExitBootServices(gImageHandle, map->key);
        if (status != EFI_INVALID_PARAMETER || attempt == 1) return status;
        /* Firmware may already have partially shut down. Refresh in the
         * existing buffer; do not log, allocate, free, or call protocols. */
        status = efi_map_refresh(map);
        if (status != EFI_SUCCESS) return status;
    }
    return status;
}

void efi_map_limits(const OPENSTEP_EFI_MAP *map, int *conv, int *ext)
{
    UINT64 low = efi_memory_end(map->data, map->size, map->stride,
                                OPENSTEP_LOW_START, 0xa0000, 0);
    UINT64 high = efi_memory_end(map->data, map->size, map->stride,
                                 OPENSTEP_HIGH_START, OPENSTEP_MEMORY_LIMIT, 0);
    *conv = (int)(low/1024);
    *ext = (int)((high-OPENSTEP_HIGH_START)/1024);
}
