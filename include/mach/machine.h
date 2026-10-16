/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988,1987 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 * HISTORY
 * $Log:	machine.h,v $
 * Revision 2.17  93/02/04  07:49:46  danner
 * 	PS2 support.
 * 	[93/01/18            prithvi]
 * 
 * Revision 2.16  93/01/14  17:44:40  danner
 * 	Cleanup.
 * 	[92/06/10            pds]
 * 	Added Alpha architecture, and some initial machine types.
 * 	[92/06/06            af]
 * 
 * Revision 2.15  92/08/03  17:18:04  jfriedl
 * 	Added pc532.
 * 	[92/05/15            jvh]
 * 
 * Revision 2.14  92/02/19  15:08:38  elf
 * 	Added more Sparc subtypes.
 * 	[92/02/19            rpd]
 * 
 * Revision 2.13  91/12/10  16:32:50  jsb
 * 	Fixes from Intel
 * 	[91/12/10  15:51:57  jsb]
 * 
 * Revision 2.12  91/07/31  17:53:56  dbg
 * 	Remove declaration of interrupt_stack - it is machine-dependent.
 * 	Changed CPU_SUBTYPE_LUNA88K - there's already a (potential)
 * 	mc88000 port.
 * 	[91/07/26            dbg]
 * 
 * Revision 2.11  91/07/09  23:22:06  danner
 * 	   Added CPU_SUBTYPE_LUNA88K
 * 	[91/05/06            danner]
 * 
 * Revision 2.10  91/05/14  16:55:24  mrt
 * 	Correcting copyright
 * 
 * Revision 2.9  91/05/08  12:49:54  dbg
 * 	Add CPU_SUBTYPE_SYMMETRY.
 * 	[91/04/26  14:44:44  dbg]
 * 
 * Revision 2.8  91/02/05  17:33:48  mrt
 * 	Changed to new Mach copyright
 * 	[91/02/01  17:18:51  mrt]
 * 
 * Revision 2.7  90/12/04  14:51:27  jsb
 * 	Renamed CPU_SUBTYPE_iPSC2 as CPU_SUBTYPE_iPSC386.
 * 	[90/12/03  22:22:25  jsb]
 * 
 * Revision 2.6  90/11/25  17:48:28  jsb
 * 	Added CPU_TYPE_I860 and CPU_SUBTYPE_iPSC860.
 * 	[90/11/25  16:51:44  jsb]
 * 
 * Revision 2.5  90/09/23  17:45:42  jsb
 * 	Added CPU_SUBTYPE_iPSC2.
 * 	[90/09/21  16:43:01  jsb]
 * 
 * Revision 2.4  90/08/07  22:23:05  rpd
 * 	Added new MIPS subtypes for R3000, fix old defs.
 * 	[90/08/07  15:54:40  af]
 * 
 * Revision 2.2.4.1  90/06/11  10:59:52  af
 * 	Added new MIPS subtypes for R3000, fix old defs.
 * 
 * Revision 2.3  90/06/02  14:58:47  rpd
 * 	Picked up new cpu types.
 * 	[90/03/26  22:34:38  rpd]
 * 
 * Revision 2.2  89/11/29  14:09:32  af
 * 	Changes for MACH_KERNEL:
 * 	. Removed should_exit.
 * 
 * 	17-May-1989 Randall W. Dean (rwd)
 * 	changed machine/vm_types.h to mach/vm_param.h
 * 	[89/05/23            dbg]
 * 
 * Revision 2.1  89/08/03  16:02:33  rwd
 * Created.
 * 
 * Revision 2.11  89/04/18  16:43:32  mwyoung
 * 	Use <machine/vm_types.h> rather than <vm/vm_param.h> to get
 * 	VM types.  Remove old history... none of it was insightful.
 * 
 * 	The variable declarations should be moved elsewhere.
 * 	[89/01/24            mwyoung]
 * 
 */
/*	File:	machine.h
 *	Author:	Avadis Tevanian, Jr.
 *	Date:	1986
 *
 *	Machine independent machine abstraction.
 */

#ifndef	_MACH_MACHINE_H_
#define _MACH_MACHINE_H_

#import <mach/machine/vm_types.h>
#import <mach/boolean.h>

/*
 *	For each host, there is a maximum possible number of
 *	cpus that may be available in the system.  This is the
 *	compile-time constant NCPUS, which is defined in cpus.h.
 *
 *	In addition, there is a machine_slot specifier for each
 *	possible cpu in the system.
 */

struct machine_info {
	integer_t	major_version;	/* kernel major version id */
	integer_t	minor_version;	/* kernel minor version id */
	integer_t	max_cpus;	/* max number of cpus compiled */
	integer_t	avail_cpus;	/* number actually available */
	vm_size_t	memory_size;	/* size of memory in bytes */
};

typedef struct machine_info	*machine_info_t;
typedef struct machine_info	machine_info_data_t;	/* bogus */

typedef integer_t	cpu_type_t;
typedef integer_t	cpu_subtype_t;

#define CPU_STATE_MAX		3

#define CPU_STATE_USER		0
#define CPU_STATE_SYSTEM	1
#define CPU_STATE_IDLE		2

struct machine_slot {
/*boolean_t*/integer_t	is_cpu;		/* is there a cpu in this slot? */
	cpu_type_t	cpu_type;	/* type of cpu */
	cpu_subtype_t	cpu_subtype;	/* subtype of cpu */
/*boolean_t*/integer_t	running;	/* is cpu running */
	integer_t	cpu_ticks[CPU_STATE_MAX];
	integer_t	clock_freq;	/* clock interrupt frequency */
};

typedef struct machine_slot	*machine_slot_t;
typedef struct machine_slot	machine_slot_data_t;	/* bogus */

#ifdef	KERNEL
extern struct machine_info	machine_info;
extern struct machine_slot	machine_slot[];

extern vm_offset_t		interrupt_stack[];
#endif	KERNEL

/*
 *	Machine types known by all.
 */
 
#define CPU_TYPE_ANY		((cpu_type_t) -1)

#define CPU_TYPE_VAX		((cpu_type_t) 1)
/* skip				((cpu_type_t) 2)	*/
/* skip				((cpu_type_t) 3)	*/
/* skip				((cpu_type_t) 4)	*/
/* skip				((cpu_type_t) 5)	*/
#define	CPU_TYPE_MC680x0	((cpu_type_t) 6)
#define CPU_TYPE_I386		((cpu_type_t) 7)
/* skip CPU_TYPE_MIPS		((cpu_type_t) 8)	*/
/* skip 			((cpu_type_t) 9)	*/
#define CPU_TYPE_MC98000	((cpu_type_t) 10)
#define CPU_TYPE_HPPA           ((cpu_type_t) 11)
/* skip CPU_TYPE_ARM		((cpu_type_t) 12)	*/
#define CPU_TYPE_MC88000	((cpu_type_t) 13)
#define CPU_TYPE_SPARC		((cpu_type_t) 14)
#define CPU_TYPE_I860		((cpu_type_t) 15)
/* skip	CPU_TYPE_ALPHA		((cpu_type_t) 16)	*/

/*
 *	Machine subtypes (these are defined here, instead of in a machine
 *	dependent directory, so that any program can get all definitions
 *	regardless of where is it compiled).
 */

/*
 *	Object files that are hand-crafted to run on any
 *	implementation of an architecture are tagged with
 *	CPU_SUBTYPE_MULTIPLE.  This functions essentially the same as
 *	the "ALL" subtype of an architecture except that it allows us
 *	to easily find object files that may need to be modified
 *	whenever a new implementation of an architecture comes out.
 *
 *	It is the responsibility of the implementor to make sure the
 *	software handles unsupported implementations elegantly.
 */
#define	CPU_SUBTYPE_MULTIPLE		((cpu_subtype_t) -1)
#define CPU_SUBTYPE_LITTLE_ENDIAN	((cpu_subtype_t) 0)
#define CPU_SUBTYPE_BIG_ENDIAN		((cpu_subtype_t) 1)

/*
 *	VAX subtypes (these do *not* necessary conform to the actual cpu
 *	ID assigned by DEC available via the SID register).
 */

#define	CPU_SUBTYPE_VAX_ALL	((cpu_subtype_t) 0) 
#define CPU_SUBTYPE_VAX780	((cpu_subtype_t) 1)
#define CPU_SUBTYPE_VAX785	((cpu_subtype_t) 2)
#define CPU_SUBTYPE_VAX750	((cpu_subtype_t) 3)
#define CPU_SUBTYPE_VAX730	((cpu_subtype_t) 4)
#define CPU_SUBTYPE_UVAXI	((cpu_subtype_t) 5)
#define CPU_SUBTYPE_UVAXII	((cpu_subtype_t) 6)
#define CPU_SUBTYPE_VAX8200	((cpu_subtype_t) 7)
#define CPU_SUBTYPE_VAX8500	((cpu_subtype_t) 8)
#define CPU_SUBTYPE_VAX8600	((cpu_subtype_t) 9)
#define CPU_SUBTYPE_VAX8650	((cpu_subtype_t) 10)
#define CPU_SUBTYPE_VAX8800	((cpu_subtype_t) 11)
#define CPU_SUBTYPE_UVAXIII	((cpu_subtype_t) 12)

/*
 * 	680x0 subtypes
 *
 * The subtype definitions here are unusual for historical reasons.
 * NeXT used to consider 68030 code as generic 68000 code.  For
 * backwards compatability:
 * 
 *	CPU_SUBTYPE_MC68030 symbol has been preserved for source code
 *	compatability.
 *
 *	CPU_SUBTYPE_MC680x0_ALL has been defined to be the same
 *	subtype as CPU_SUBTYPE_MC68030 for binary comatability.
 *
 *	CPU_SUBTYPE_MC68030_ONLY has been added to allow new object
 *	files to be tagged as containing 68030-specific instructions.
 */

#define	CPU_SUBTYPE_MC680x0_ALL		((cpu_subtype_t) 1)
#define CPU_SUBTYPE_MC68030		((cpu_subtype_t) 1) /* compat */
#define CPU_SUBTYPE_MC68040		((cpu_subtype_t) 2) 
#define	CPU_SUBTYPE_MC68030_ONLY	((cpu_subtype_t) 3)

/*
 *	I386 subtypes.
 */

#define	CPU_SUBTYPE_I386_ALL	((cpu_subtype_t) 3)
#define CPU_SUBTYPE_386		((cpu_subtype_t) 3)
#define CPU_SUBTYPE_486		((cpu_subtype_t) 4)
#define CPU_SUBTYPE_486SX	((cpu_subtype_t) 4 + 128)
#define CPU_SUBTYPE_586		((cpu_subtype_t) 5)
#define CPU_SUBTYPE_586SX	((cpu_subtype_t) 5 + 128)

/*
 *	Mips subtypes.
 */

#define	CPU_SUBTYPE_MIPS_ALL	((cpu_subtype_t) 0)
#define CPU_SUBTYPE_MIPS_R2300	((cpu_subtype_t) 1)
#define CPU_SUBTYPE_MIPS_R2600	((cpu_subtype_t) 2)
#define CPU_SUBTYPE_MIPS_R2800	((cpu_subtype_t) 3)
#define CPU_SUBTYPE_MIPS_R2000a	((cpu_subtype_t) 4)	/* pmax */
#define CPU_SUBTYPE_MIPS_R2000	((cpu_subtype_t) 5)
#define CPU_SUBTYPE_MIPS_R3000a	((cpu_subtype_t) 6)	/* 3max */
#define CPU_SUBTYPE_MIPS_R3000	((cpu_subtype_t) 7)

/*
 *	MC98000 (PowerPC) subtypes
 */
#define	CPU_SUBTYPE_MC98000_ALL	((cpu_subtype_t) 0)
#define CPU_SUBTYPE_MC98601	((cpu_subtype_t) 1)

/*
 *	HPPA subtypes for Hewlett-Packard HP-PA family of
 *	risc processors. Port by NeXT to 700 series. 
 */

#define	CPU_SUBTYPE_HPPA_ALL		((cpu_subtype_t) 0)
#define CPU_SUBTYPE_HPPA_7100		((cpu_subtype_t) 0) /* compat */
#define CPU_SUBTYPE_HPPA_7100LC		((cpu_subtype_t) 1)

/*
 *	MC88000 subtypes.
 */
#define	CPU_SUBTYPE_MC88000_ALL	((cpu_subtype_t) 0)
#define CPU_SUBTYPE_MC88100	((cpu_subtype_t) 1)
#define CPU_SUBTYPE_MC88110	((cpu_subtype_t) 2)

/*
 *	SPARC subtypes
 */
#define	CPU_SUBTYPE_SPARC_ALL		((cpu_subtype_t) 0)

/*
 *	I860 subtypes
 */
#define CPU_SUBTYPE_I860_ALL	((cpu_subtype_t) 0)
#define CPU_SUBTYPE_I860_860	((cpu_subtype_t) 1)

#endif	_MACH_MACHINE_H_
