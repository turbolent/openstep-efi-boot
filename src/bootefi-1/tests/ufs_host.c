/* Windows host shim below the actual boot-2 disk/UFS reader. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <io.h>
#include <fcntl.h>

char intbuf_backing[18 * 512];
unsigned char uses_ebios[8] = {1, 1, 1, 1, 1, 1, 1, 1};
static unsigned int bootstruct[16384];
void *kernBootStruct = bootstruct;
static FILE *disk;

extern int sa_open(char *, int);
extern int sa_read(int, char *, int);
extern int sa_close(int);

void *sa_malloc(unsigned int size) { return calloc(1, size); }
int ebiosread(int biosdev, int sector, int count)
{
    size_t bytes = count * 512U;
    if (biosdev != 0x80 || bytes > sizeof(intbuf_backing) ||
        _fseeki64(disk, (__int64)(unsigned int)sector * 512, SEEK_SET))
        return -1;
    memset(intbuf_backing, 0, bytes);
    return fread(intbuf_backing, 1, bytes, disk) == bytes ? 0 : -1;
}
int biosread(int a, int b, int c, int d, int e) { return -1; }
long get_diskinfo(int device) { return 0xf3f; }
void turnOffFloppy(void) { }
void spinActivityIndicator(void) { }
void clearActivityIndicator(void) { }
void sleep(int seconds) { }
void halt(void) { exit(1); }
void panic(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    exit(1);
}
int verbose(const char *fmt, ...) { return 0; }
int error(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    return 0;
}
void bcopy(const void *src, void *dest, unsigned int count)
{
    memmove(dest, src, count);
}
int ffs(int value)
{
    unsigned int bits = (unsigned int)value;
    int i = 1;
    if (!bits) return 0;
    while (!(bits & 1)) { bits >>= 1; ++i; }
    return i;
}
int ptol(char *s)
{
    int c = *s;
    if (c >= '0' && c <= '7') return c - '0';
    if (c >= 'a' && c <= 'h') return c - 'a';
    return 0;
}
int main(int argc, char **argv)
{
    char spec[1024], buffer[65536];
    int fd, count;
    if (argc != 3) return 2;
    disk = fopen(argv[1], "rb");
    if (!disk) return 2;
    bootstruct[0xa8 / 4] = 1; /* numIDEs, checked in abi.c */
    _setmode(_fileno(stdout), _O_BINARY);
    if (snprintf(spec, sizeof(spec), "hd(0,a)%s", argv[2]) >= sizeof(spec)) return 2;
    fd = sa_open(spec, 0);
    if (fd < 0) return 1;
    while ((count = sa_read(fd, buffer, sizeof(buffer))) > 0)
        if (fwrite(buffer, 1, count, stdout) != count) return 2;
    sa_close(fd);
    fclose(disk);
    return count < 0 ? 1 : 0;
}
