#ifndef OPENSTEP_FIRMWARE_CONSOLE_H
#define OPENSTEP_FIRMWARE_CONSOLE_H
#include "efi.h"

typedef struct {
    void *protocol;
    UINT32 original_mode;
    int restore;
} FIRMWARE_CONSOLE;

EFI_STATUS firmware_console_text(FIRMWARE_CONSOLE *);
void firmware_console_restore(FIRMWARE_CONSOLE *);
#endif
