#ifndef _SYS_CYGWIN_H
#define _SYS_CYGWIN_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

extern pid_t cygwin32_winpid_to_pid (int);
extern void cygwin32_win32_to_posix_path_list (const char*, char*);
extern int cygwin32_win32_to_posix_path_list_buf_size (const char*);
extern void cygwin32_posix_to_win32_path_list (const char*, char*);
extern int cygwin32_posix_to_win32_path_list_buf_size (const char*);

#ifdef __cplusplus
};
#endif

#endif /* _SYS_CYGWIN_H */
