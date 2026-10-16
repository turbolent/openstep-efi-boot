#ifndef OPENSTEP_EFI_RESERVATIONS_H
#define OPENSTEP_EFI_RESERVATIONS_H
#include "efi.h"

EFI_STATUS efi_reserve_ranges(void);
void efi_release_ranges(void);
void efi_reservation_wait(void);
#endif
