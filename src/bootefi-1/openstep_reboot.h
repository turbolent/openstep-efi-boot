#ifndef OPENSTEP_REBOOT_H
#define OPENSTEP_REBOOT_H
typedef const unsigned char *(*OPENSTEP_ACPI_READ)(void *, unsigned int, unsigned int);
/* Return only a validated, byte-wide System I/O reset register. */
int openstep_acpi_reset(unsigned int rsdp, OPENSTEP_ACPI_READ read, void *context,
                        unsigned short *port, unsigned char *value);
int openstep_fix_reboot(unsigned char *kernel, unsigned int size,
                       unsigned short port, unsigned char value);
void openstep_prepare_reboot(unsigned char *kernel, unsigned int size);
#endif
