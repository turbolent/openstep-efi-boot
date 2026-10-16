/* Copyright (c) 1992 NeXT Computer, Inc. */

#ifndef _ANSI_LIMITS_H
#define _ANSI_LIMITS_H

#include <standards.h>

#include <ansi/machine/limits.h>

#ifdef _POSIX_SOURCE
#define ARG_MAX		40960  /* Length of arguments to    */ 
			       /* exec (include environ)    */
#define CHILD_MAX	100    /* processes per uid         */
#define OPEN_MAX	256    /* open files per process    */
#define MAX_CANON	256    /* bytes	in a terminal       */
			       /* canonical input line.     */
#define MAX_INPUT	512    /* Minimum number of bytes   */
	 		       /* for which space will be   */
			       /* available in a terminal   */
	         	       /* input queue. 	            */
#define LINK_MAX	32767  /* File's link count.        */	
#define NAME_MAX	255    /* Bytes in a file name      */
#define NGROUPS_MAX	16     /* Number of simultaneous    */  
	                       /* supplementary group IDs   */
			       /* per process.              */
#define PATH_MAX	1024   /* Bytes in a pathname       */  
#define PIPE_BUF	4096   /* Bytes that can be written */
			       /* atomically to a pipe.     */
#define SSIZE_MAX	INT_MAX	/* Value that can be stored */
			        /* in object of type ssize_t.*/ 
#define STREAM_MAX	256    /* Open streams per process  */
#define TZNAME_MAX	50     /* bytes for time zone name  */	

#define _POSIX_ARG_MAX		4096
#define _POSIX_CHILD_MAX	6
#define _POSIX_LINK_MAX		8
#define _POSIX_MAX_CANON	255
#define _POSIX_MAX_INPUT	255
#define _POSIX_NAME_MAX		14
#define _POSIX_NGROUPS_MAX	0
#define _POSIX_OPEN_MAX		16
#define _POSIX_PATH_MAX		255
#define _POSIX_PIPE_BUF		512
#define _POSIX_SSIZE_MAX	32767
#define _POSIX_STREAM_MAX	8
#define _POSIX_TZNAME_MAX	3
#endif /* _POSIX_SOURCE */

#endif /* _ANSI_LIMITS_H */
