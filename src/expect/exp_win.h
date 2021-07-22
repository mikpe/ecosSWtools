/* exp_win.h - window support

Written by: Don Libes, NIST, 10/25/93

This file is in the public domain.  However, the author and NIST
would appreciate credit if you use this file or parts of it.
*/

#include "expect_comm.h"

EXTERN void exp_window_size_set _ANSI_ARGS_ ((int));
EXTERN void exp_window_size_get _ANSI_ARGS_ ((int));

EXTERN void exp_win_rows_set _ANSI_ARGS_ ((char *));
EXTERN void exp_win_rows_get _ANSI_ARGS_ ((char *));
EXTERN void exp_win_columns_set _ANSI_ARGS_ ((char *));
EXTERN void exp_win_columns_get _ANSI_ARGS_ ((char *));

EXTERN void exp_win2_rows_set _ANSI_ARGS_ ((int, char *));
EXTERN void exp_win2_rows_get _ANSI_ARGS_ ((int, char *));
EXTERN void exp_win2_columns_set _ANSI_ARGS_ ((int, char *));
EXTERN void exp_win2_columns_get _ANSI_ARGS_ ((int, char *));
