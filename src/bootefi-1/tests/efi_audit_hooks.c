/* QEMU-only adversarial firmware shim. Never included in the USB payload.
 * Require aligned disk reads and change the map on the first exit attempt. */
#include "efi.h"
static EFI_BOOT_SERVICES *services;
static EFI_STATUS (*original_protocol)(EFI_HANDLE, EFI_GUID *, void **);
static EFI_STATUS (*original_exit)(EFI_HANDLE, UINTN);
typedef EFI_STATUS (*READ)(EFI_BLOCK_IO_PROTOCOL *, UINT32, UINT64, UINTN, void *);
static struct { EFI_BLOCK_IO_PROTOCOL *bio; READ read; } devices[32];
static unsigned count;
/* Read by the host test using the fixture link map. */
volatile UINT32 efi_audit_stats[5] = {0x41554454, 0, 0, 0, 0};

static EFI_STATUS aligned_read(EFI_BLOCK_IO_PROTOCOL *bio, UINT32 media,
                               UINT64 lba, UINTN size, void *buffer)
{
    unsigned i;
    if ((UINTN)buffer & 4095) { efi_audit_stats[2]++; return EFI_INVALID_PARAMETER; }
    efi_audit_stats[1]++;
    for (i = 0; i < count; i++)
        if (devices[i].bio == bio) return devices[i].read(bio, media, lba, size, buffer);
    return EFI_NOT_FOUND;
}
static EFI_STATUS protocol(EFI_HANDLE handle, EFI_GUID *guid, void **out)
{
    EFI_STATUS status = original_protocol(handle, guid, out);
    static const EFI_GUID block = EFI_BLOCK_IO_PROTOCOL_GUID;
    unsigned i;
    if (efi_audit_stats[3]) { efi_audit_stats[4]++; return EFI_UNSUPPORTED; }
    if (status != EFI_SUCCESS) return status;
    for (i = 0; i < sizeof(block); i++)
        if (((UINT8 *)guid)[i] != ((const UINT8 *)&block)[i]) return status;
    for (i = 0; i < count; i++) if (devices[i].bio == *out) return status;
    if (count == 32) return EFI_OUT_OF_RESOURCES;
    devices[count].bio = *out;
    devices[count].read = devices[count].bio->ReadBlocks;
    devices[count].bio->ReadBlocks = aligned_read;
    devices[count].bio->Media->IoAlign = 4096;
    count++;
    return status;
}
static EFI_STATUS leave(EFI_HANDLE image, UINTN key)
{
    efi_audit_stats[3]++;
    if (efi_audit_stats[3] == 1) {
        UINT64 address = 0x4000000;
        EFI_STATUS status = services->AllocatePages(AllocateAddress, EfiACPIMemoryNVS, 1, &address);
        unsigned i;
        if (status != EFI_SUCCESS) { efi_audit_stats[4]++; return status; }
        for (i = 0; i < 4096; i++) ((volatile UINT8 *)(UINTN)address)[i] = 0x5a;
        return EFI_INVALID_PARAMETER;
    }
    return original_exit(image, key);
}
void efi_audit_install(EFI_BOOT_SERVICES *bs)
{
    UINT32 crc = ~0U, size = ((UINT32 *)bs->Hdr)[3];
    unsigned i, bit;
    services = bs; original_protocol = bs->HandleProtocol; original_exit = bs->ExitBootServices;
    bs->HandleProtocol = protocol; bs->ExitBootServices = leave;
    ((UINT32 *)bs->Hdr)[4] = 0;
    for (i = 0; i < size; i++) {
        crc ^= ((UINT8 *)bs)[i];
        for (bit = 0; bit < 8; bit++) crc = (crc >> 1) ^ ((crc & 1) ? 0xedb88320U : 0);
    }
    ((UINT32 *)bs->Hdr)[4] = ~crc;
}
