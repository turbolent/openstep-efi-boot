/* QEMU-only fixture: create the MacBook7,1 low-memory hole, fill it with a
 * sentinel, then run the exact delivered EFI64 loader through firmware. */
#include "efi.h"
#include "payload.h"

typedef EFI_STATUS (*LOAD)(BOOLEAN, EFI_HANDLE, void *, void *, UINTN, EFI_HANDLE *);
typedef EFI_STATUS (*START)(EFI_HANDLE, UINTN *, CHAR16 **);

EFI_STATUS efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *table)
{
    EFI_BOOT_SERVICES *bs = table->BootServices;
    EFI_LOADED_IMAGE_PROTOCOL *loaded = 0;
    EFI_GUID image_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_GUID path_guid = EFI_DEVICE_PATH_PROTOCOL_GUID;
    EFI_STATUS status;
    EFI_HANDLE child = 0;
    EFI_PHYSICAL_ADDRESS occupied = 0x8f000;
    volatile unsigned char *sentinel;
    unsigned i;
    void *path = 0;
    status = bs->AllocatePages(AllocateAddress, EfiACPIMemoryNVS, 1, &occupied);
    if (EFI_ERROR(status)) return status;
    sentinel = (void *)(UINTN)occupied;
    for (i = 0; i < 4096; i++) sentinel[i] = 0xa5;
    table->ConOut->OutputString(table->ConOut,
        L"TEST: ACPI NVS page at 0x8F000 filled with A5.\r\n");
    status = bs->HandleProtocol(image, &image_guid, (void **)&loaded);
    if (!EFI_ERROR(status)) status = bs->HandleProtocol(loaded->DeviceHandle, &path_guid, &path);
    if (!EFI_ERROR(status)) status = ((LOAD)bs->LoadImage)(0, image, path, (void *)payload, sizeof(payload), &child);
#ifdef EFI_AUDIT_STRESS
    if (!EFI_ERROR(status)) {
        extern void efi_audit_install(EFI_BOOT_SERVICES *);
        efi_audit_install(bs);
    }
#endif
    if (!EFI_ERROR(status)) status = ((START)bs->StartImage)(child, 0, 0);
    /* Successful boot never returns here. */
    bs->FreePages(occupied, 1);
    return status;
}
