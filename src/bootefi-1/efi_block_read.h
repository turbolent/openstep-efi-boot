#ifndef OPENSTEP_EFI_BLOCK_READ_H
#define OPENSTEP_EFI_BLOCK_READ_H
#include "efi.h"
EFI_STATUS efi_block_read(EFI_BLOCK_IO_PROTOCOL *bio, UINT64 lba,
                           UINTN size, void *destination);
#endif
