/* OPENSTEP's spurious IRQ15 path must acknowledge the master's IRQ2.
 * The stock path returns without that EOI, blocking later slave IRQs.
 * Match the complete known sequence before changing the loaded copy.
 * See the quickstep and patch_kernel_pic.py references in README.
 */
#include "openstep_pic.h"

static const struct {
    unsigned int offset;
    unsigned char code[30];
} profiles[] = {
    { 0x8c70a, { /* OPENSTEP 4.2 */
        0x7d,0x0f,0x83,0xfb,0x0f,0x75,0x17,0xba,0xa0,0x00,0x00,0x00,
        0xec,0x84,0xc0,0x7c,0x0d,0xff,0x05,0xd8,0x69,0x1f,0x00,
        0xe9,0x26,0x01,0x00,0x00,0x90,0x90 } },
    { 0x8c88e, { /* OPENSTEP 4.2 Patch 4 */
        0x7d,0x0f,0x83,0xfb,0x0f,0x75,0x17,0xba,0xa0,0x00,0x00,0x00,
        0xec,0x84,0xc0,0x7c,0x0d,0xff,0x05,0x18,0x7a,0x1f,0x00,
        0xe9,0x26,0x01,0x00,0x00,0x90,0x90 } }
};

int openstep_fix_pic(unsigned char *kernel, unsigned int size)
{
    unsigned int p, i;
    unsigned char replacement[30];
    static const unsigned char tail[] = {
        0xb0,0x62,0xe6,0x20, /* mov al,0x62; out 0x20,al */
        0xe9,0x28,0x01,0x00,0x00,0x90,0x90,0x90,0x90
    };
    if (size < 8 || kernel[0] != 0xce || kernel[1] != 0xfa ||
        kernel[2] != 0xed || kernel[3] != 0xfe || kernel[4] != 7 ||
        kernel[5] || kernel[6] || kernel[7])
        return 0;
    for (p = 0; p < sizeof(profiles) / sizeof(profiles[0]); p++) {
        unsigned char *site;
        int stock = 1, fixed = 1;
        if (profiles[p].offset > size || size - profiles[p].offset < 30)
            continue;
        site = kernel + profiles[p].offset;
        for (i = 0; i < 17; i++)
            replacement[i] = profiles[p].code[i];
        replacement[1] = 0x13; /* spurious IRQ7 skips the cascade EOI */
        for (i = 0; i < sizeof(tail); i++)
            replacement[17 + i] = tail[i];
        for (i = 0; i < 30; i++) {
            if (site[i] != profiles[p].code[i])
                stock = 0;
            if (site[i] != replacement[i])
                fixed = 0;
        }
        if (fixed)
            return 2;
        if (stock) {
            for (i = 0; i < 30; i++)
                site[i] = replacement[i];
            return 1;
        }
    }
    return 0;
}
