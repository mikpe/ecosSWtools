/* exp_tty.h - tty support definitions

Design and implementation of this program was paid for by U.S. tax
dollars.  Therefore it is public domain.  However, the author and NIST
would appreciate credit if this program or parts of it are used.
*/

#ifndef __EXP_TTY_H__
#define __EXP_TTY_H__

#include "expect_cf.h"
#include "expect_comm.h"

extern int exp_dev_tty;
extern int exp_ioctled_devtty;
extern int exp_stdin_is_tty;
extern int exp_stdout_is_tty;

EXTERN void exp_tty_raw _ANSI_ARGS_ ((int));
EXTERN void exp_tty_echo _ANSI_ARGS_ ((int));
EXTERN void exp_tty_break _ANSI_ARGS_ ((Tcl_Interp *, int));
EXTERN int exp_tty_raw_noecho _ANSI_ARGS_ ((Tcl_Interp *, exp_tty *, int *, int *));
EXTERN int exp_israw _ANSI_ARGS_ ((void));
EXTERN int exp_isecho _ANSI_ARGS_ ((void));

EXTERN void exp_tty_set _ANSI_ARGS_ ((Tcl_Interp *, exp_tty *, int, int));
EXTERN int exp_tty_set_simple _ANSI_ARGS_ ((exp_tty *));
EXTERN int exp_tty_get_simple _ANSI_ARGS_ ((exp_tty *));

#endif	/* __EXP_TTY_H__ */
