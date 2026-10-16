/* Compile boot-2 disk I/O with scratch memory outside page zero,
 * which OVMF protects, and above the full OPENSTEP boot structure.
 * boot.c's separate EISA configuration buffer is not used by this loader.
 * Force-include this header before any source includes memory.h. */
#include <memory.h>

/* The full OPENSTEP config area ends at 0x204fc. Keep disk scratch
 * above it and below the standalone linker at 0x30000. */
#undef BIOS_ADDR
#define BIOS_ADDR	0x022000
