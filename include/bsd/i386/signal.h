/*
 * Copyright (c) 1992 NeXT Computer, Inc.
 *
 * HISTORY
 *
 * 8 April 1992 David E. Bohman at NeXT
 *	Created.
 */

#ifndef	_i386_SIGNAL_
#define	_i386_SIGNAL_ 1

/*
 * Information pushed on stack when a signal is delivered.
 * This is used by the kernel to restore state following
 * execution of the signal handler.  It is also made available
 * to the handler to allow it to properly restore state if
 * a non-standard exit is performed.
 */
struct	sigcontext {
    int			sc_onstack;	/* sigstack state to restore */
    int			sc_mask;	/* signal mask to restore */
    unsigned int	sc_eax;
    unsigned int	sc_ebx;
    unsigned int	sc_ecx;
    unsigned int	sc_edx;
    unsigned int	sc_edi;
    unsigned int	sc_esi;
    unsigned int	sc_ebp;
    unsigned int	sc_esp;
    unsigned int	sc_ss;
    unsigned int	sc_eflags;
    unsigned int	sc_eip;
    unsigned int	sc_cs;
    unsigned int	sc_ds;
    unsigned int	sc_es;
    unsigned int	sc_fs;
    unsigned int	sc_gs;
};

#endif	_i386_SIGNAL_
