#ifndef OPENSTEP_EFI_MAP_H
#define OPENSTEP_EFI_MAP_H
#include "efi.h"

typedef struct {
    EFI_MEMORY_DESCRIPTOR *data;
    UINTN size, capacity, key, stride;
    UINT32 version;
} OPENSTEP_EFI_MAP;

/* acquire owns a pool buffer on success; refresh never allocates. */
EFI_STATUS efi_map_acquire(OPENSTEP_EFI_MAP *map);
EFI_STATUS efi_map_refresh(OPENSTEP_EFI_MAP *map);
/* Keep the returned buffer after an exit attempt, even on failure. No
 * console/protocol calls are safe once *attempted is set. */
EFI_STATUS efi_exit_services(OPENSTEP_EFI_MAP *map, int *attempted);
void efi_map_limits(const OPENSTEP_EFI_MAP *map, int *conv, int *ext);
#endif
