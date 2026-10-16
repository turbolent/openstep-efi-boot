#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "openstep_pic.h"

int main(void)
{
    unsigned char *kernel = calloc(1, 0x100000);
    unsigned char *saved = malloc(0x100000);
    unsigned int profile;
    const unsigned int offsets[] = { 0x8c70a, 0x8c88e };
    unsigned char stock[] = {
        0x7d,0x0f,0x83,0xfb,0x0f,0x75,0x17,0xba,0xa0,0x00,0x00,0x00,
        0xec,0x84,0xc0,0x7c,0x0d,0xff,0x05,0xd8,0x69,0x1f,0x00,
        0xe9,0x26,0x01,0x00,0x00,0x90,0x90
    };
    assert(kernel && saved);
    for (profile = 0; profile < 2; profile++) {
        unsigned int off = offsets[profile];
        memset(kernel, 0xa5, 0x100000);
        memcpy(kernel, "\xce\xfa\xed\xfe\x07\0\0\0", 8);
        stock[19] = profile ? 0x18 : 0xd8;
        stock[20] = profile ? 0x7a : 0x69;
        memcpy(kernel + off, stock, sizeof(stock));
        memcpy(saved, kernel, 0x100000);
        assert(openstep_fix_pic(kernel, off + sizeof(stock) - 1) == 0);
        assert(memcmp(saved, kernel, 0x100000) == 0);
        kernel[off + 12] ^= 1;
        assert(openstep_fix_pic(kernel, 0x100000) == 0);
        kernel[off + 12] ^= 1;
        assert(openstep_fix_pic(kernel, 0x100000) == 1);
        assert(memcmp(saved, kernel, off) == 0);
        assert(memcmp(saved + off + 30, kernel + off + 30,
                      0x100000 - off - 30) == 0);
        /* IRQ7 branches past the new master-only EOI. IRQ15 sends 0x62.
         * Both branches still land on the original common return path. */
        assert(kernel[off + 1] == 0x13);
        assert(memcmp(kernel + off + 17, "\xb0\x62\xe6\x20", 4) == 0);
        assert(kernel[off + 21] == 0xe9 && kernel[off + 22] == 0x28);
        assert(off + 23 + 5 + 0x126 == off + 21 + 5 + 0x128);
        memcpy(saved, kernel, 0x100000);
        assert(openstep_fix_pic(kernel, 0x100000) == 2);
        assert(memcmp(saved, kernel, 0x100000) == 0);
        kernel[4] = 18;
        assert(openstep_fix_pic(kernel, 0x100000) == 0);
    }
    free(saved);
    free(kernel);
    return 0;
}
