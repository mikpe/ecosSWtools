/* exp_pty.h - declarations for pty allocation and testing

Written by: Don Libes, NIST,  3/9/93

Design and implementation of this program was paid for by U.S. tax
dollars.  Therefore it is public domain.  However, the author and NIST
would appreciate credit if this program or parts of it are used.

*/

EXTERN int exp_pty_test_start _ANSI_ARGS_ ((void));
EXTERN void exp_pty_test_end _ANSI_ARGS_ ((void));
EXTERN int exp_pty_test _ANSI_ARGS_ ((char *, char *, int, char *));
EXTERN void exp_pty_unlock _ANSI_ARGS_ ((void));
EXTERN int exp_pty_lock _ANSI_ARGS_ ((int, char *));

extern char *exp_pty_slave_name;
