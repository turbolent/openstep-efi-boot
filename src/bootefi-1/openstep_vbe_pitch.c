/* Patch 4's VBE converter sets totalWidth = visible width. DriverKit's
 * software cursors use totalWidth, while window drawing uses rowBytes.
 * EFI exposes exactly one mode, so its storage depth also applies to every
 * record passed to this converter. Correct only the loaded kernel copy.
 */
#include "openstep_vbe_pitch.h"

int openstep_fix_vbe_pitch(unsigned char *kernel, unsigned int size,
                          unsigned int depth)
{
    static const unsigned char prefix[] = {
        0x55,0x89,0xe5,0x57,0x56,0x53,0x8b,0x5d,0x08,
        0x8b,0x75,0x0c,0x0f,0xb7,0x7b,0x04,0x89,0x3e,
        0x0f,0xb7,0x7b,0x06,0x89,0x7e,0x04
    };
    static const unsigned char stock[] = {
        0x0f,0xb7,0x7b,0x04,0x89,0x7e,0x08,
        0x0f,0xb7,0x7b,0x08,0x89,0x7e,0x0c
    };
    unsigned char fixed[] = {
        0x0f,0xb7,0x7b,0x08, /* movzx edi, word [mode+8] */
        0x89,0x7e,0x0c,      /* info->rowBytes = edi */
        0xc1,0xef,0x02,      /* shr edi, log2(storage bytes per pixel) */
        0x89,0x7e,0x08,0x90 /* info->totalWidth = edi */
    };
    unsigned int i;
    unsigned char *site;
    int original = 1, patched = 1;
    if ((depth != 16 && depth != 32) || size < 0x9edb3 ||
        kernel[0] != 0xce || kernel[1] != 0xfa || kernel[2] != 0xed ||
        kernel[3] != 0xfe || kernel[4] != 7 || kernel[5] ||
        kernel[6] || kernel[7])
        return 0;
    for (i = 0; i < sizeof(prefix); i++)
        if (kernel[0x9ed8c + i] != prefix[i]) return 0;
    fixed[9] = depth == 32 ? 2 : 1;
    site = kernel + 0x9eda5;
    for (i = 0; i < sizeof(stock); i++) {
        if (site[i] != stock[i]) original = 0;
        if (site[i] != fixed[i]) patched = 0;
    }
    if (patched) return 2;
    if (!original) return 0;
    for (i = 0; i < sizeof(fixed); i++) site[i] = fixed[i];
    return 1;
}
