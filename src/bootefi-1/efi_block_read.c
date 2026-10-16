/* Block I/O requires the destination to meet Media->IoAlign, including
 * small label probes. EFI pool allocation alone promises only 8 bytes. */
#include "efi_block_read.h"

EFI_STATUS efi_block_read(EFI_BLOCK_IO_PROTOCOL *bio, UINT64 lba,
                           UINTN size, void *destination)
{
    UINTN alignment = bio->Media->IoAlign, address, i;
    void *allocation = 0;
    UINT8 *buffer, *out = destination;
    EFI_STATUS status;
    if (alignment > 1 && (alignment & (alignment-1))) return EFI_UNSUPPORTED;
    if (alignment <= 1 || !((UINTN)destination & (alignment-1)))
        return bio->ReadBlocks(bio, bio->Media->MediaId, lba, size, destination);
    if (size > (UINTN)-1 - (alignment-1)) return EFI_OUT_OF_RESOURCES;
    status = gBS->AllocatePool(EfiLoaderData, size+alignment-1, &allocation);
    if (status != EFI_SUCCESS) return status;
    address = ((UINTN)allocation+alignment-1) & ~(alignment-1);
    buffer = (void *)address;
    status = bio->ReadBlocks(bio, bio->Media->MediaId, lba, size, buffer);
    if (status == EFI_SUCCESS)
        for (i = 0; i < size; i++) out[i] = buffer[i];
    gBS->FreePool(allocation);
    return status;
}
