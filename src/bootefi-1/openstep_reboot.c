/* Patch 4's final reset uses the legacy keyboard controller. On EFI systems
 * use the FADT's reset register first, after the normal kernel shutdown.
 * All injected code lives inside the kernel, not reclaimed loader memory.
 */
#include "openstep_reboot.h"

static unsigned int u32(const unsigned char *p)
{ return p[0] | (unsigned)p[1]<<8 | (unsigned)p[2]<<16 | (unsigned)p[3]<<24; }
static int equal(const unsigned char *p, const unsigned char *q, unsigned n)
{ while (n--) if (*p++ != *q++) return 0; return 1; }
static int checksum(const unsigned char *p, unsigned n)
{ unsigned char sum = 0; while (n--) sum += *p++; return sum == 0; }

static const unsigned char *table(unsigned address, OPENSTEP_ACPI_READ read,
                                 void *context, const char *signature)
{
    const unsigned char *p = read(context, address, 36);
    unsigned size;
    if (!p || !equal(p, (const unsigned char *)signature, 4)) return 0;
    size = u32(p+4);
    if (size < 36 || size > 1024*1024) return 0;
    p = read(context, address, size);
    return p && checksum(p, size) ? p : 0;
}

static int root_reset(unsigned address, unsigned stride, OPENSTEP_ACPI_READ read,
                      void *context, unsigned short *port, unsigned char *value)
{
    const unsigned char *root = table(address, read, context, stride == 8 ? "XSDT" : "RSDT");
    unsigned i, size;
    if (!root) return 0;
    size = u32(root+4);
    if ((size-36) % stride) return 0;
    for (i=36; i<size; i+=stride) {
        const unsigned char *fadt;
        unsigned reg;
        if (stride == 8 && u32(root+i+4)) continue; /* IA32 cannot map >4 GiB. */
        fadt = table(u32(root+i), read, context, "FACP");
        if (!fadt || u32(fadt+4) < 129 || fadt[8] < 2 ||
            !(u32(fadt+112) & (1U<<10))) continue;
        /* GAS: System I/O, eight bits, offset zero, byte/unspecified access. */
        if (fadt[116] != 1 || fadt[117] != 8 || fadt[118] || fadt[119] > 1 ||
            u32(fadt+124)) continue;
        reg = u32(fadt+120);
        if (!reg || reg > 65535) continue;
        *port = (unsigned short)reg;
        *value = fadt[128];
        return 1;
    }
    return 0;
}

int openstep_acpi_reset(unsigned rsdp, OPENSTEP_ACPI_READ read, void *context,
                        unsigned short *port, unsigned char *value)
{
    const unsigned char *p = read(context, rsdp, 20);
    unsigned rsdt;
    if (!p || !equal(p, (const unsigned char *)"RSD PTR ", 8) || !checksum(p, 20)) return 0;
    rsdt = u32(p+16);
    if (p[15] >= 2) {
        unsigned length;
        p = read(context, rsdp, 36);
        if (!p) return 0;
        length = u32(p+20);
        if (length < 36 || length > 4096) return 0;
        p = read(context, rsdp, length);
        if (!p || !checksum(p, length)) return 0;
        if (!u32(p+28) && root_reset(u32(p+24), 8, read, context, port, value)) return 1;
    }
    return root_reset(rsdt, 4, read, context, port, value);
}

int openstep_fix_reboot(unsigned char *kernel, unsigned size,
                       unsigned short port, unsigned char value)
{
    static const unsigned char original[] = {
        0x55,0x89,0xe5,0xa1,0x78,0x4b,0x1e,0,0x85,0xc0,0x74,4,0xff,0xd0,0x90,0x90,
        0x6a,0x64,0xe8,0x35,0xbc,0xfe,0xff,0x83,0xc4,4,0xa8,2,0x75,0xf2,
        0x68,0xfe,0,0,0,0x6a,0x64,0xe8,0x5e,0xbc,0xfe,0xff,0x89,0xec,0x5d,0xc3
    };
    static const unsigned char caller[] = {0xe8,0x52,0xe3,0xff,0xff,0xe8,0xc9,0x3b,1,0,0x90,0xf4,0xeb,0xfd};
    static const unsigned char delay[] = {0x55,0x89,0xe5,0x8b,0x4d,8,0xeb,0x0f};
    unsigned char fixed[] = {
        0x55,0x89,0xe5,0xfa,             /* frame; CLI */
        0x66,0xba,0,0,0xb0,0,0xee,     /* mov dx,port; mov al,value; out dx,al */
        0x68,0x98,0x3a,0,0,             /* push 15000 microseconds */
        0xe8,0x3f,0xc4,0xfe,0xff,      /* call Patch 4's us_spin (0x18d1e4) */
        0x83,0xc4,4,
        0xb9,0xa0,0x86,1,0,            /* bounded 8042 fallback: 100000 polls */
        0xe4,0x64,0xa8,2,0xe0,0xfa,0x75,4,
        0xb0,0xfe,0xe6,0x64,0xc9,0xc3,0x90,0x90,0x90
    };
    unsigned i;
    unsigned char *site;
    if (!port || size < 0xa0dbe || u32(kernel) != 0xfeedface || u32(kernel+4) != 7 ||
        !equal(kernel+0x8d1bd, caller, sizeof(caller)) ||
        !equal(kernel+0x8d1e4, delay, sizeof(delay))) return 0;
    fixed[6] = (unsigned char)port; fixed[7] = (unsigned char)(port>>8); fixed[9] = value;
    site = kernel+0xa0d90;
    if (equal(site, fixed, sizeof(fixed))) return 2;
    if (!equal(site, original, sizeof(original))) return 0;
    for (i=0; i<sizeof(fixed); i++) site[i] = fixed[i];
    return 1;
}
