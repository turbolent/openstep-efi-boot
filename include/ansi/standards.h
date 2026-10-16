/* Copyright (c) 1992 NeXT, Inc. - 11/14/92 WMG */

#ifndef _STANDARDS_H
#define _STANDARDS_H

#ifdef _POSIX_SOURCE
	#ifndef __STRICT_ANSI__
		#define __STRICT_ANSI__
	#endif
	#undef __STRICT_BSD__
#endif /* _POSIX_SOURCE */

#endif /* _STANDARDS_H */
