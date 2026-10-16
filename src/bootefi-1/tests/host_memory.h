extern char intbuf_backing[18 * 512];
#undef BIOS_ADDR
#define BIOS_ADDR intbuf_backing
