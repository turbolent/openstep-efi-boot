/* 
 * Copyright (c) 1987 NeXT, Inc.
 *
 * HISTORY
 *
 * 20 April 1992 David E. Bohman at NeXT
 *	Created from 68k version.
 * 21-Nov-88  Avadis Tevanian (avie) at NeXT
 *	Moved assertion definitions to machine independent <kern/assert.h>.
 *
 * 09-Nov-86  John Seamons (jks) at NeXT
 *	Ported to NeXT.
 */ 

#ifndef	_I386_MACHPARAM_
#define	_I386_MACHPARAM_

#import <mach/vm_param.h>

/*
 * Machine dependent 'constants'.
 */
#define	NBPG	PAGE_SIZE	/* bytes/page */
#define	PGOFSET	(NBPG-1)	/* byte offset into page */
#define	PGSHIFT	PAGE_SHIFT	/* LOG2(NBPG) */

#define	CLSIZE			1
#define	CLSIZELOG2		0

/*
 * Some macros for units conversion
 */
/* Core clicks (NeXT_page_size bytes) to segments and vice versa */
#define	ctos(x)	(x)
#define	stoc(x)	(x)

/* clicks to bytes */
#define	ctob(x)	((x) << PGSHIFT)

/* bytes to clicks */
#define	btoc(x)	((((unsigned)(x)+PGOFSET) >> PGSHIFT))

/*
 * Macros to decode processor status word.
 */
#define STATUS_WORD(rpl, ipl)	(((ipl) << 8) | (rpl))
#define	USERMODE(x)		(((x) & 3) == 3)
#define	BASEPRI(x)		(((x) & (255 << 8)) == 0)

#if	defined(KERNEL) || defined(STANDALONE)
#define	DELAY(n) us_spin(n)

#else	defined(KERNEL) || defined(STANDALONE)
#define	DELAY(n)	{ register int N = (n); while (--N > 0); }
#endif	defined(KERNEL) || defined(STANDALONE)

#endif	_I386_MACHPARAM_
