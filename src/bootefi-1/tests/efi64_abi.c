#include <stddef.h>
#include "efi64_bridge.h"
#define AT(type, field, offset) _Static_assert(offsetof(type, field) == offset, #type "." #field)
_Static_assert(sizeof(UINTN) == 8 && sizeof(void *) == 8, "native EFI64 pointers");
_Static_assert(EFI_BUFFER_TOO_SMALL == 0x8000000000000005ULL, "64-bit EFI status");
AT(EFI_SYSTEM_TABLE, ConOut, 64);
AT(EFI_SYSTEM_TABLE, BootServices, 96);
AT(EFI_BOOT_SERVICES, AllocatePages, 40);
AT(EFI_BOOT_SERVICES, HandleProtocol, 152);
AT(EFI_BOOT_SERVICES, ExitBootServices, 232);
AT(EFI_BLOCK_IO_PROTOCOL, ReadBlocks, 24);
AT(EFI_BLOCK_IO_MEDIA, LastBlock, 24);
AT(OPENSTEP_GOP_MODE, FrameBufferBase, 24);
AT(OPENSTEP_GOP_MODE, FrameBufferSize, 32);
AT(EFI64_BRIDGE, code32, 6);
AT(EFI64_BRIDGE, data, 8);
AT(OPENSTEP_FRAMEBUFFER, base, 40);
AT(EFI32_BLOCK, media_value, 32);
AT(EFI32_BLOCK, read, 16);
AT(EFI32_BLOCK, handle, 64);
AT(EFI32_IMAGE, device, 12);
_Static_assert(sizeof(OPENSTEP_FRAMEBUFFER) == 56, "fixed-width framebuffer bridge");
