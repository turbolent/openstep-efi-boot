/*
 * Copyright (c) 1992 NeXT Computer, Inc.
 *
 * Intel386 Family:	Segment selector.
 *
 * HISTORY
 *
 * 29 March 1992 David E. Bohman at NeXT
 *	Created.
 */

/*
 * Segment selector.
 */

typedef struct sel {
    unsigned short	rpl	:2,
#define KERN_PRIV	0
#define USER_PRIV	3
			ti	:1,
#define SEL_GDT		0
#define SEL_LDT		1
			index	:13;
} sel_t;

#define NULL_SEL	((sel_t) { 0, 0, 0 } )
