/*
 * Copyright (c) 1992 NeXT Computer, Inc.
 *
 * Simple spin locks.
 *
 * HISTORY
 *
 * 19 November 1992 David E. Bohman at NeXT
 *	Created.
 */
 
#import <mach/boolean.h>

#ifndef	_MACH_I386_SIMPLE_LOCK_H_
#define _MACH_I386_SIMPLE_LOCK_H_

#define _MACHINE_SIMPLE_LOCK_DATA_

struct slock {
    boolean_t		locked;
};

typedef struct slock		simple_lock_data_t;
typedef simple_lock_data_t	*simple_lock_t;

static __inline__
void
simple_lock_init(
    simple_lock_t	slock
)
{
    slock->locked = FALSE;
}

static __inline__
boolean_t
simple_lock_try(
    simple_lock_t	slock
)
{
    boolean_t		result;
    
    asm volatile(
    	"xchgl %1,%0; xorl %3,%0"
	    : "=r" (result), "=m" (slock->locked)
	    : "0" (TRUE), "i" (TRUE));
	    
    return (result);
}

static __inline__
void
simple_lock(
    simple_lock_t	slock
)
{    
    do
    	{
	    while (slock->locked)
		continue;
	}
    while (!simple_lock_try(slock));
}

static __inline__
void
simple_unlock(
    simple_lock_t	slock
)
{
    boolean_t		result;
    
    asm volatile(
	"xchgl %1,%0"
	    : "=r" (result), "=m" (slock->locked)
	    : "0" (FALSE));
}

#endif	_MACH_I386_SIMPLE_LOCK_H_
