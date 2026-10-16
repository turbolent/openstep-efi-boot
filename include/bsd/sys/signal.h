/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * HISTORY
 *  7-Jan-93  Mac Gillon (mgillon) at NeXT
 *	Integrated POSIX changes
 *
 * 21-May-89  Avadis Tevanian, Jr. (avie) at NeXT, Inc.
 *	Purged all crud that had accumulated in this file.  Machine specific
 *	definitions now go in <machine/signal.h> (including sigcontext).
 */

/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)signal.h	7.1 (Berkeley) 6/4/86
 */

#ifndef _SYS_SIGNAL_H
#define _SYS_SIGNAL_H

#if !defined(KERNEL) && defined(_POSIX_SOURCE)
#include <standards.h>
#endif

#ifdef _NEXT_SOURCE
	#import <machine/signal.h>
	#define NSIG	32
#endif

#if defined(_POSIX_SOURCE) || defined(_NEXT_SOURCE)
	#define	SIGHUP	1	/* hangup */
	#define	SIGINT	2	/* interrupt */
	#define	SIGQUIT	3	/* quit */
	#define	SIGILL	4	/* illegal instruction (not */
					/* reset when  caught) */
#endif /* _POSIX_SOURCE || _NEXT_SOURCE */

#ifdef _NEXT_SOURCE
/* CHME, CHMS, CHMU are not yet given back to users 
* reasonably
*/
	#define	SIGTRAP	5 	/* trace trap (not reset */
#endif /* _NEXT_SOURCE */

#if defined(_POSIX_SOURCE) || defined(_NEXT_SOURCE)
	#define	SIGABRT	6	/* IOT instruction */
#endif /* _POSIX_SOURCE || _NEXT_SOURCE */

#ifdef _NEXT_SOURCE
	#define	SIGIOT	SIGABRT	/* compatibility */
	#define	SIGEMT	7	/* EMT instruction */
#endif /* _NEXT_SOURCE */

#if defined(_POSIX_SOURCE) || defined(_NEXT_SOURCE)
	#define	SIGFPE	8	/* floating point exception */
	#define	SIGKILL	9	/* kill (cannot be caught */
			        /* or ignored) */
#endif /* _POSIX_SOURCE || _NEXT_SOURCE */

#ifdef _NEXT_SOURCE
	#define	SIGBUS	10	/* bus error */
#endif /* _NEXT_SOURCE */

#if defined(_POSIX_SOURCE) || defined(_NEXT_SOURCE)
	#define	SIGSEGV	11	/* segmentation violation */
#endif /* _POSIX_SOURCE || _NEXT_SOURCE */

#ifdef _NEXT_SOURCE
	#define	SIGSYS	12	/* bad argument to system */
				/* call */
#endif /* _NEXT_SOURCE */

#if defined(_POSIX_SOURCE) || defined(_NEXT_SOURCE)
	#define	SIGPIPE	13	/* write on a pipe with no */
				/* one to read  it */
	#define	SIGALRM	14	/* alarm clock */
	#define	SIGTERM	15	/* software termination */
				/* signal from kill */
#endif /* _POSIX_SOURCE || _NEXT_SOURCE */

#ifdef _NEXT_SOURCE
	#define	SIGURG	16	/* urgent condition on IO */
				/* channel */
#endif /* _NEXT_SOURCE */

#if defined(_POSIX_SOURCE) || defined(_NEXT_SOURCE)
	#define	SIGSTOP	17	/* sendable stop signal */
				/* not from tty */
	#define	SIGTSTP	18	/* stop signal from tty */
	#define	SIGCONT	19	/* continue a stopped process */
	#define	SIGCHLD	20	/* to parent on child stop */
				/* or exit */
#endif /* _POSIX_SOURCE || _NEXT_SOURCE */

#ifdef _NEXT_SOURCE
	#define	SIGCLD	SIGCHLD	/* compatibility */
#endif /* _NEXT_SOURCE */

#if defined(_POSIX_SOURCE) || defined(_NEXT_SOURCE)
	#define	SIGTTIN	21	/* to readers pgrp upon */
				/* background tty read */
	#define	SIGTTOU	22	/* like TTIN for output */
				 /* if (tp->t_local&LTOSTOP) */
#endif /* _POSIX_SOURCE || _NEXT_SOURCE */

#ifdef _NEXT_SOURCE
	#define	SIGIO	23	/* input/output possible */
				/* signal */
	#define	SIGXCPU	24	/* exceeded CPU time limit */
	#define	SIGXFSZ	25	/* exceeded file size limit */
	#define	SIGVTALRM 26	/* virtual time alarm */
	#define	SIGPROF	27	/* profiling time alarm */
	#define SIGWINCH 28	/* window size changes */
	/* SUN_LOCK*/
	#define	SIGLOST 29	/* resource lost 
				 * (eg, record-lock lost) */
		/* SUN_LOCK*/
#endif /* _NEXT_SOURCE */

#define SIGUSR1 30	/* user defined signal 1 */
#define SIGUSR2 31	/* user defined signal 2 */

#if	!defined(__STDC__) && !defined(KERNEL)
int	(*signal())();
#endif	!defined(__STDC__) && !defined(KERNEL)

#ifdef _NEXT_SOURCE
	#if	!defined(__STDC__) && !defined(KERNEL)
		int	(*signal())();
	#endif

	/*
	 * Signal vector "template" used in sigvec call.
	 */
	#ifndef	__ASSEMBLER__
		struct	sigvec {
			void	(*sv_handler)();  /* signal handler */
			int	sv_mask; /* signal mask to apply */
			int	sv_flags; /* see signal options below */
			};
	#endif	/* __ASSEMBLER__ */
	#define SV_ONSTACK	0x0001	/* take signal on signal */
					/* stack */
	#define SV_INTERRUPT	0x0002  /* do not restart system */
					/* on signal return */
	#define sv_onstack sv_flags	/* isn't compatibility */
						/* wonderful! */
	/*
	 * Structure used in sigstack call.
	*/
	#ifndef	__ASSEMBLER__
		struct	sigstack {
			char	*ss_sp;	    /* signal stack pointer */
			int	ss_onstack; /* current status */
		};
	#endif	__ASSEMBLER__
#endif /* _NEXT_SOURCE */

#ifdef __STRICT_BSD__
	#define	BADSIG		(int (*)())-1
	#define	SIG_DFL		(int (*)())0
	#define	SIG_IGN		(int (*)())1

	#ifdef KERNEL
		#define	SIG_CATCH	(int (*)())2
		#define	SIG_HOLD	(int (*)())3
	#endif

#else /* __STRICT_BSD__ */

	#ifdef _NEXT_SOURCE 
		#define	BADSIG		(void (*)())-1
	#endif /* _NEXT_SOURCE */

	#if defined(_NEXT_SOURCE) || defined(_POSIX_SOURCE)
		#define	SIG_DFL		(void (*)())0
		#define	SIG_IGN		(void (*)())1
	#endif /* _NEXT_SOURCE || _POSIX_SOURCE  */


	#ifdef KERNEL
		#define	SIG_CATCH	(void (*)())2
		#define	SIG_HOLD	(void (*)())3
	#endif

#endif /* __STRICT_BSD__ */

#ifdef _NEXT_SOURCE
	#if	defined(__STDC__) && !defined(KERNEL)
		/* man(2) declarations */
		extern int sigblock(int);
		extern int sigpause(int);
		extern int sigreturn(struct sigcontext *);
		extern int sigsetmask(int);
		extern int sigstack(struct sigstack *, \
					struct sigstack *);
		extern int sigvec(int, struct sigvec *, \
					struct sigvec *);
		/* man(3) declarations */
		extern int siginterrupt(int, int);
		#ifdef __STRICT_BSD__
			extern int (*signal(int, int (*)()))();
		#else
			extern void (*signal(int, \
				void (*)(int)))(int);
		#endif
	#endif	/* __STDC__ && KERNEL */

	/*
	 * Macro for converting signal number to a mask suitable for
	 * sigblock().
	 */
	#define sigmask(m)	(1 << ((m)-1))

	#if	KERNEL
		/*
		 *	signals delivered on a per-thread basis.
		 */
		#define threadmask (sigmask(SIGILL)|sigmask(SIGTRAP)|\
				    sigmask(SIGIOT)|sigmask(SIGEMT)|\
				    sigmask(SIGFPE)|sigmask(SIGBUS)|\
				    sigmask(SIGSEGV)|sigmask(SIGSYS)|\
				    sigmask(SIGPIPE))
	#endif	KERNEL
#endif /* _NEXT_SOURCE */

#ifdef _POSIX_SOURCE		/* Additional entries required by */
				/* POSIX 			  */

	#include <sys/types.h>	/* for pid_t */

	typedef int sigset_t; 

	struct sigaction {
		void		(*sa_handler)();
		sigset_t	sa_mask;
		int		sa_flags;
	};

	#define SIG_BLOCK	00
	#define SIG_UNBLOCK	01
	#define SIG_SETMASK	02

	#define SA_NOCLDSTOP	04

	#if defined(__STDC__) && !defined(KERNEL)
		extern int kill(pid_t pid, int sig);
		extern int sigaction(int sig, \
		                     const struct sigaction *act, \
		                     struct sigaction *oact);
		extern int sigaddset(sigset_t *set, int signo);
		extern int sigdelset(sigset_t *set, int signo);
		extern int sigemptyset(sigset_t *set);
		extern int sigfillset(sigset_t *set);
		extern int sigismember(const sigset_t *set, int signo);
		extern int sigpending(sigset_t *set);
		extern int sigprocmask(int how, const sigset_t *set, \
		                       sigset_t *oset);
		extern int sigsuspend(const sigset_t *sigmask);
	#else /* __STDC__ */
		extern int kill();
		extern int sigaction();
		extern int sigaddset();
		extern int sigdelset();
		extern int sigemptyset();
		extern int sigfillset();
		extern int sigismember();
		extern int sigpending();
		extern int sigprocmask();
		extern int sigsuspend();
	#endif /* __STDC__ */

#endif /* _POSIX_SOURCE */
#endif /* _SYS_SIGNAL_H */
