#ifndef OPENSTEP_EFI64_BRIDGE_H
#define OPENSTEP_EFI64_BRIDGE_H
#include "firmware_graphics.h"

enum {
    BR_ALLOC, BR_FREE, BR_MAP, BR_PAGES, BR_LOCATE, BR_PROTOCOL, BR_READ,
    BR_OUTPUT, BR_INPUT, BR_STALL, BR_WATCHDOG, BR_EXIT, BR_GRAPHICS, BR_CHECKPOINT
};
typedef struct {
    UINT32 gate;
    UINT16 code64, code32, data, reserved;
    UINT32 image, tables, table_count;
} EFI64_BRIDGE;
/* ABI layouts consumed by the IA32 adapters. Function pointers and handles
 * here are 32-bit values even when this header is compiled for EFI64. */
typedef struct {
    UINT64 revision;
    UINT32 media, reset, read, write, flush, padding;
    EFI_BLOCK_IO_MEDIA media_value;
    UINT32 handle;
} EFI32_BLOCK;
typedef struct {
    UINT32 revision, parent, system_table, device;
} EFI32_IMAGE;
#ifndef OPENSTEP_EFI64
UINT32 bridge_call(UINT32 operation, ...);
#endif
#endif
