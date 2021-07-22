#include <_ansi.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <errno.h>
#include "swi.h"

#ifdef ARM_RDI_MONITOR
/* Adjust our internal handles to stay away from std* handles */
#define FILE_HANDLE_OFFSET (0x20)
static int angel_stdin;
static int angel_stdout;
static int angel_stderr;

static inline int
do_AngelSWI (int reason, void * arg)
{
  int value;
  asm volatile ("mov r0, %1; mov r1, %2; swi %a3; mov %0, r0"
       : "=r" (value) /* Outputs */
       : "r" (reason), "r" (arg), "i" (AngelSWI) /* Inputs */
       : "r0", "r1", "lr"
		/* Clobbers r0 and r1, and lr if in supervisor mode */);
  return value;
}

/* Function to convert std(in|out|err) handles to internal versions */
static int
remap_handle (int fh)
{
  if (fh == __sfileno (stdin))
    return angel_stdin;
  if (fh == __sfileno (stdout))
    return angel_stdout;
  if (fh == __sfileno (stderr))
    return angel_stderr;

  return fh - FILE_HANDLE_OFFSET;
}

void
initialise_angel_handles (void)
{
  int block[3];
  
  block[0] = (int) ":tt";
  block[2] = 3;     /* length of filename */
  block[1] = 0;     /* mode "r" */
  angel_stdin = do_AngelSWI (AngelSWI_Reason_Open, block);

  block[0] = (int) ":tt";
  block[2] = 3;     /* length of filename */
  block[1] = 4;     /* mode "w" */
  angel_stdout = angel_stderr = do_AngelSWI (AngelSWI_Reason_Open, block);
}

#endif /* ARM_RDI_MONITOR */

static int
get_errno ()
{
#ifdef ARM_RDI_MONITOR
  return do_AngelSWI (AngelSWI_Reason_Errno, NULL);
#else
  asm ("swi %a0" :: "i" (SWI_GetErrno));
#endif
}

static int
error (int result)
{
  errno = get_errno ();
  return result;
}

static int
wrap (int result)
{
  if (result == -1)
    return error (-1);
  return result;
}

/* Returns # chars not! written */

int
_swiread (int file,
       char *ptr,
       int len)
{
#ifdef ARM_RDI_MONITOR
  int block[3];
  
  block[0] = remap_handle (file);
  block[1] = (int) ptr;
  block[2] = len;
  
  return do_AngelSWI (AngelSWI_Reason_Read, block);
#else
  asm ("swi %a0" : : "i" (SWI_Read));
#endif
}

int
_read (int file,
       char *ptr,
       int len)
{
  int x = _swiread (file, ptr, len);

  if (x < 0)
    return error (-1);

  /* x == len is not an error, at least if we want feof() to work */
  return len - x;
}

int
_swilseek (int file,
	int ptr,
	int dir)
{
#ifdef ARM_RDI_MONITOR
  /* This only does absolute seeks */
  int block[2], res;
  
  block[0] = remap_handle (file);
  block[1] = ptr;
  
  res = do_AngelSWI (AngelSWI_Reason_Seek, block);

  /* This is expected to return the position in the file */
  return res == 0 ? ptr : -1;
#else
  asm ("swi %a0" :: "i" (SWI_Seek));
#endif
}

int
_lseek (int file,
	int ptr,
	int dir)
{
  return wrap (_swilseek (file, ptr, dir));
}

static
writechar (char c)
{
#ifdef ARM_RDI_MONITOR
  return do_AngelSWI (AngelSWI_Reason_WriteC, & c);
#else
  asm ("swi %a0" :: "i" (SWI_WriteC));
#endif
}

/* Returns #chars not! written */

int
_swiwrite (
	 int file,
	 char *ptr,
	 int len)
{
#ifdef ARM_RDI_MONITOR
  int block[3];
  
  block[0] = remap_handle (file);
  block[1] = (int) ptr;
  block[2] = len;
  return do_AngelSWI (AngelSWI_Reason_Write, block);
#else
  asm ("swi %a0" :: "i" (SWI_Write));
#endif
}

int
_write (int file,
	char *ptr,
	int len)
{
  int x = _swiwrite (file, ptr,len);

  if (x == -1 || x == len)
    return error (-1);
  return len - x;
}

int
_swiopen (const char *path,
       int flags)
{
#ifdef ARM_RDI_MONITOR
  int block[3],aflags=0, fh;
  
  block[0] = (int) path;
  block[2] = strlen (path);

  /* The flags are Unix-style, so we need to convert them */
#ifdef O_BINARY
  if (flags & O_BINARY)
    aflags |= 1;
#endif

  if (flags & O_RDWR)
    aflags |= 2;

  if (flags & O_CREAT)
    aflags |= 4;

  if (flags & O_APPEND)
    aflags |= 8;
  
  block[1] = aflags;
  
  fh = do_AngelSWI (AngelSWI_Reason_Open, block);
  
  return fh >= 0 ? fh + FILE_HANDLE_OFFSET : error (fh);
#else
  asm ("swi %a0" :: "i" (SWI_Open));
#endif
}

int
_open (const char *path,
       int flags,...)
{
  return wrap (_swiopen (path, flags));
}

int
_swiclose (int file)
{
#ifdef ARM_RDI_MONITOR
  int myhan = remap_handle (file);
  
  return do_AngelSWI (AngelSWI_Reason_Close, & myhan);
#else
  asm ("swi %a0" :: "i" (SWI_Close));
#endif
}

int
_close (int file)
{
  return wrap (_swiclose (file));
}

void
_exit (n)
{
#ifdef ARM_RDI_MONITOR
  do_AngelSWI (AngelSWI_Reason_ReportException,
	      (void *) ADP_Stopped_ApplicationExit);
#else
  asm ("swi %a0" :: "i" (SWI_Exit));
#endif
}

void
abort ()
{
#ifdef ARM_RDI_MONITOR
  do_AngelSWI (AngelSWI_Reason_ReportException,
	      (void *) ADP_Stopped_RunTimeError);
#else
 asm ("mov r0,#17\nswi %a0" :: "i" (SWI_Exit));
#endif
}

int
_kill (n, m)
{
#ifdef ARM_RDI_MONITOR
  do_AngelSWI (AngelSWI_Reason_ReportException,
	      (void *) ADP_Stopped_ApplicationExit);
#else
  asm ("swi %a0" :: "i" (SWI_Exit));
#endif
}

int
_getpid (n)
{
  return 1;
}

register char *stack_ptr asm ("sp");

caddr_t
_sbrk (int incr)
{
  extern char end asm ("__end__");	/* Defined by the linker */
  static char *heap_end;
  char *prev_heap_end;

  if (heap_end == NULL)
    {
      heap_end = & end;
    }
  
  prev_heap_end = heap_end;
  
  if (heap_end + incr > stack_ptr)
    {
      _write (1, "_sbrk: Heap and stack collision\n", 32);
      abort ();
    }
  
  heap_end += incr;

  return (caddr_t) prev_heap_end;
}

int
_fstat (int file,
	struct stat *st)
{
  st->st_mode = S_IFCHR;
  return 0;
}

int
_unlink ()
{
  return -1;
}

isatty (fd)
     int fd;
{
  return 1;
}

_raise ()
{
}

#if 0
int
_stat (const char *path, struct stat *st)
{
  asm ("swi %a0" :: "i" (SWI_Stat));
}

int
_chmod (const char *path, short mode)
{
  asm ("swi %a0" :: "i" (SWI_Chmod));
}

int
_chown (const char *path, short owner, short group)
{
  asm ("swi %a0" :: "i" (SWI_Chown));
}

int
_utime (path, times)
     const char *path;
     char *times;
{
  asm ("swi %a0" :: "i" (SWI_Utime));
}

int
_fork ()
{
  asm ("swi %a0" :: "i" (SWI_Fork));
}

int
_wait (statusp)
     int *statusp;
{
  asm ("swi %a0" :: "i" (SWI_Wait));
}

int
_execve (const char *path, char *const argv[], char *const envp[])
{
  return _trap3 (SYS_execve, path, argv, envp);
}

int
_execv (const char *path, char *const argv[])
{
  return _trap3 (SYS_execv, path, argv);
}

int
_pipe (int *fd)
{
  return _trap3 (SYS_pipe, fd);
}
#endif

alarm()
{
}
