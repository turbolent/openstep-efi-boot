#ifndef OPENSTEP_GOP_H
#define OPENSTEP_GOP_H

#include "efi.h"
#include "kernBootStruct.h"
#include "firmware_graphics.h"

/* A boot-record identifier, not a callable BIOS mode number. */
#define OPENSTEP_GOP_VBE_MODE 0x1ff

int openstep_gop_record(const OPENSTEP_GOP_INFO *, UINT64 base, UINT32 size,
                        boot_vbe_mode *out);
void openstep_prepare_graphics(void);
int openstep_graphics_active(void);
void openstep_init_graphics(void);
void openstep_fb_init(const boot_vbe_mode *);
void openstep_fb_putc(int);
void openstep_fb_status(const char *, unsigned int);

#endif
