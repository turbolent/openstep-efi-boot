/*
 * Copyright (c) 1992 NeXT Computer, Inc.
 *
 * Intel386 Family:	Virtual memory constants.
 *
 * HISTORY
 *
 * 14 April 1992 David E. Bohman at NeXT
 *	Created.
 */

#ifndef	_MACH_I386_VM_PARAM_H_
#define _MACH_I386_VM_PARAM_H_

#import <sys/types.h>

#define BYTE_SIZE	8	/* byte size in bits */
#define BYTE_MSF	0

#define I386_PGBYTES	4096	/* bytes per 80386 page */
#define I386_PGSHIFT	12	/* number of bits to shift for pages */

/*
 *	Convert bytes to pages and convert pages to bytes.
 *	No rounding is used.
 */

#define i386_btop(x)		(((unsigned)(x)) >> I386_PGSHIFT)
#define i386_ptob(x)		(((unsigned)(x)) << I386_PGSHIFT)

/*
 *	Round off or truncate to the nearest page.  These will work
 *	for either addresses or counts.  (i.e. 1 byte rounds to 1 page
 *	bytes.
 */

#define i386_round_page(x)	((((unsigned)(x)) + I386_PGBYTES - 1) & \
					~(I386_PGBYTES-1))
#define i386_trunc_page(x)	(((unsigned)(x)) & ~(I386_PGBYTES-1))

#define VM_MIN_ADDRESS		((vm_offset_t) 0)
#define VM_MAX_ADDRESS		((vm_offset_t) 0xc0000000)

#define VM_MIN_KERNEL_ADDRESS	((vm_offset_t) 0x00000000)
#define VM_MAX_KERNEL_ADDRESS	((vm_offset_t) 0x40000000)

#define KERNSTACK_SIZE		(1*I386_PGBYTES)
#define INTSTACK_SIZE		(1*I386_PGBYTES)

/*
 *	Conversion between 80386 pages and VM pages
 */

#define trunc_i386_to_vm(p)	(atop(trunc_page(i386_ptob(p))))
#define round_i386_to_vm(p)	(atop(round_page(i386_ptob(p))))
#define vm_to_i386(p)		(i386_btop(ptoa(p)))

/*
 * Maximum alignment required by any data type for this architecture.
 * (Use 4 bytes for performance reasons....)
 */
#define	MAX_DATA_ALIGNMENT	4		/* 4 bytes */

#endif	_MACH_I386_VM_PARAM_H_
