/*
 * Copyright (c) 1992 NeXT Computer, Inc.
 *
 * Intel386 Family:	For setjmp/longjmp (kernel version).
 *
 * HISTORY
 *
 * 20 April 1992 David E. Bohman at NeXT
 *	Created.
 */
 
#ifndef _BSD_I386_LABEL_T_H_
#define _BSD_I386_LABEL_T_H_

typedef struct label_t {
	int	val[14];
} label_t;

#endif	_BSD_I386_LABEL_T_H_
