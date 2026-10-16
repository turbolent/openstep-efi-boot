#include "efi_map.h"
#include "openstep_reboot.h"
extern unsigned int efi_acpi_rsdp(void);
extern int printf(const char *, ...);
extern int verbose(const char *, ...);

static const unsigned char *read_acpi(void *context, unsigned address, unsigned size)
{
    OPENSTEP_EFI_MAP *map = context;
    UINTN offset;
    UINT64 end = (UINT64)address + size;
    if (!address || end > 0x100000000ULL) return 0;
    for (offset=0; offset<map->size; offset+=map->stride) {
        const EFI_MEMORY_DESCRIPTOR *d = (const void *)((const UINT8 *)map->data+offset);
        /* Do not follow a malformed ACPI pointer into MMIO or free memory. */
        if (d->Type != EfiACPIReclaimMemory && d->Type != EfiACPIMemoryNVS &&
            d->Type != EfiBootServicesData && d->Type != EfiRuntimeServicesData) continue;
        if (address >= d->PhysicalStart && end >= address &&
            end <= d->PhysicalStart+d->NumberOfPages*4096)
            return (const unsigned char *)(UINTN)address;
    }
    return 0;
}

void openstep_prepare_reboot(unsigned char *kernel, unsigned size)
{
    OPENSTEP_EFI_MAP map;
    unsigned short port = 0;
    unsigned char value = 0;
    int patched = 0;
    if (efi_map_acquire(&map) == EFI_SUCCESS) {
        if (openstep_acpi_reset(efi_acpi_rsdp(), read_acpi, &map, &port, &value))
            patched = openstep_fix_reboot(kernel, size, port, value);
        gBS->FreePool(map.data);
    }
    if (patched)
        verbose("OPENSTEP reboot: ACPI I/O port %x, value %x (RAM patch)\n", port, value);
    else
        verbose("OPENSTEP reboot: no supported ACPI reset register/kernel; stock reset retained\n");
}
