/*
 * ARCH_INCLUDE.h -- helper macro for including architecture specific
 * header files.
 *
 * Usage:
 *	#import	ARCH_INCLUDE(prefix_dir/, suffix.h)
 * Where:
 *	ARCH_INCLUDE(prefix_dir/, suffix.h)
 * produces (for m68k architecture):
 *	"prefix_dir/m68k/suffix.h"
 *
 * NOTE: prefix_dir, if non-null, must end with a '/'
 *
 * Current deficiency: can't specify <...>, only "...".
 *
 * __TARGET_ARCHITECTURE__ may be defined to specify a different
 * different architecture that the compiler is generating code for.
 * Define __TARGET_ARCHITECTURE__ as a string, e.g.:
 #	#define	__TARGET_ARCHITECTURE__	"m68k"
 */

#ifndef	_ARCH_INCLUDE_H_
#define	_ARCH_INCLUDE_H_

#ifndef	__TARGET_ARCHITECTURE__
#define	__TARGET_ARCHITECTURE__	__ARCHITECTURE__
#endif	__TARGET_ARCHITECTURE__

#define	ARCH_INCLUDE(prefix, suffix)	\
	#prefix __TARGET_ARCHITECTURE__ "/" #suffix

#endif	_ARCH_INCLUDE_H_
