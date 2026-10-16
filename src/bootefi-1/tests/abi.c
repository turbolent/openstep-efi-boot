/* Offsets read by the OPENSTEP 4.2 i386 kernel and VBE driver binaries. */
#include <stddef.h>
#include "bootefi_memory_override.h"
#include "../openstep_gop.h"

#define OFFSET(field, expected) \
    _Static_assert(offsetof(KERNBOOTSTRUCT, field) == (expected), #field)
OFFSET(bootString, 0x2);
OFFSET(magicCookie, 0xa4);
OFFSET(numIDEs, 0xa8);
OFFSET(convmem, 0xb0);
OFFSET(extmem, 0xb4);
OFFSET(first_addr0, 0x138);
OFFSET(graphicsMode, 0x14c);
OFFSET(numBootDrivers, 0x154);
OFFSET(configEnd, 0x158);
OFFSET(vbeFrameBuffer, 0x1854);
OFFSET(vbeCurrentMode, 0x1858);
OFFSET(vbeModes, 0x1870);
OFFSET(pciInfo, 0x20f0);
OFFSET(eisaConfigFunctions, 0x20f8);
OFFSET(eisaSlotInfo, 0x20fc);
OFFSET(config, 0x24fc);
_Static_assert(0x11000 + sizeof(KERNBOOTSTRUCT) <= BIOS_ADDR,
               "disk scratch must not overlap boot structure");
_Static_assert(BIOS_ADDR + BIOS_LEN <= RLD_ADDR,
               "disk scratch must not overlap standalone linker");
_Static_assert(sizeof(OPENSTEP_GOP_INFO) == 36, "GOP information ABI");
_Static_assert(offsetof(OPENSTEP_GOP_MODE, FrameBufferBase) == 16, "GOP base ABI");
_Static_assert(offsetof(OPENSTEP_GOP_MODE, FrameBufferSize) == 24, "GOP size ABI");
