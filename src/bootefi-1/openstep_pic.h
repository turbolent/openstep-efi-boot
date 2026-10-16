#ifndef OPENSTEP_PIC_H
#define OPENSTEP_PIC_H

/* 1: fixed, 2: already fixed, 0: unrecognized kernel; never writes on 0. */
int openstep_fix_pic(unsigned char *kernel, unsigned int size);

#endif
