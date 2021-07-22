/* dcrt0.cc -- a bit of crt0 that lives in the dll

   Copyright 1996, 1997, 1998 Cygnus Solutions.

This file is part of Cygwin32.

This software is a copyrighted work licensed under the terms of the
Cygwin32 license.  Please consult the file "CYGWIN32_LICENSE" for
details. */

#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include "winsup.h"
#include "glob.h"
#include "exceptions.h"
#include "dll_init.h"

per_process NO_COPY *user_data;

BOOL display_title = FALSE;
BOOL strip_title_path = FALSE;
BOOL allow_glob = TRUE;

extern "C"
{
  /* This is an exported copy of environ which can be used by DLLs
     which use cygwin.dll.  */
  char **__cygwin_environ;
  /* __progname used in getopt error message */
  char *__progname;
  struct _reent reent_data;
};

static const int NO_COPY cygwin_dll_version_major = CYGWIN_DLL_VERSION_MAJOR;
static const int NO_COPY cygwin_dll_version_minor = CYGWIN_DLL_VERSION_MINOR;

char NO_COPY *old_title = NULL;
static NO_COPY char title_buf[TITLESIZE + 1];

host_dependent_constants NO_COPY host_dependent;

/* Constructor for host_dependent_constants.  */
   
void
host_dependent_constants::init (void)
{
  /* fhandler_disk_file::lock needs a platform specific upper word
     value for locking entire files.

     fhandler_base::open requires host dependent file sharing
     attributes.  */

  switch (get_os_type ())
    {
    case winNT:
      win32_upper = 0xffffffff;
      shared = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
      break;
      
    case win95:
    case win32s:
      win32_upper = 0x00000000;
      shared = FILE_SHARE_READ | FILE_SHARE_WRITE;
      break;
      
    default:
      api_fatal ("unrecognized system type\n");
    }
}

/* Save the program name.  It's used in debugging messages and by
   the fork code (forking spawns a copy of us).  Copy it into a temp and
   then into the final spot because debugging messages use
   myself->progname. Try getting the absolute path from the
   module handle, if this fails get the name from the path.
   This call references $PATH so we can't do this until the environment
   vars are set up.  */
/* FIXME: What if argv[0] is relative, $PATH changes, and then the program
   tries to do a fork?  */

void
getprogname (char *argv0)
{
  char tmp[MAX_PATH];

  if (user_data->hmodule != 0)
    {
      if (GetModuleFileName (user_data->hmodule, tmp, MAX_PATH) == 0)
	find_exec (argv0, tmp);
    }
  else
    find_exec (argv0, tmp);
  strcpy (myself->progname, tmp);
}

/* Build argv from string passed from Windows.  */

static void
build_argv (char *in, char **out, int len)
{
  int count = 0;
  char *end = in + len;
  char *start;

  for (start = in; start < end; start++)
    {
      debug_printf ("fill iteration %s\n", start);

      /* Delete leading spaces.  */
      while (*start == ' ' || *start == '\t')
	start++;

      if (!*start)
	break;

      /* Got to an arg.  */

      char *d;
      out[count] = d = start;
      
      if (*start == '\"')
	{
	  start++;
	  while (start[0] && (start[0] != '\"' ||
                          (start[0] == '\"' && start[1] == '\"')))
	    {
	      if (*start== '\"')
		start++;
	      *d++ = *start++;
	    }
	}
      else
	{
	  while (*start && *start != ' ' && *start != '\t')
	    {
	      *d++ = *start++;
	    }
	}
      *d++ = 0;
      debug_printf ("argv[%d]=%s\n", count, out[count]);
      count++;
    }
  out[count] = 0;

  /* FIXME: Verify count == len?  */
}

/* Compute argc.
   Windows passes just a string which we need to break up into argc/argv.
   This pass computes argc.  */

static int
compute_argc (char *in)
{
  char *src;
  int count=0;

  debug_printf ("compute_argc (%s)\n", in);

  for (src = in; *src; src++)
    {
      while (*src == ' ' || *src == '\t' )
	src++;
      
      if (!*src)
	break;
      
      debug_printf ("src is now %s\n", src);
      if (*src == '\"')
	{
	  src++;
	  while (src[0])
	    {
	      if (src[0] == '\"') 
		{
		  src++;
		  if (src[0] != '\"') break;
		}
	      
	      src++;
	    }
	}
      else
	{
	  while (*src != ' ' && *src != '\t' && *src != 0)
	    src++;
	}
      src--;
      count++;
    }
  debug_printf ("count is %d\n", count);
  return count;
}

/* Perform various start up sanity checks.  */

static void
checkout (void)
{
#ifdef __PPC__
  jmp_buf x;

  if (sizeof (x) != (32 * 8))
    {
      small_printf ("Configuration error with setjmp\n");
    }
#endif
}

static void
globify (int *acp, char ***avp)
{
  /* Yes I know I could use references, but they hide the side effects */
  int ac = *acp;
  char **av = *avp;
  glob_t globs[ac];
  int rc[ac];
  int i;
  int newac = 0;
  int donesomething = 0;

  for (i = 0; i < ac; i++)
    {
      if (strpbrk (av[i], "?*[") != NULL)
	{
	  globs[i].gl_offs = 0;
	  rc[i] = glob (av[i], 
			GLOB_NOCHECK /* | GLOB_NOESCAPE */,
			0,
			&globs[i]);
	  newac += rc[i] == 0 ? globs[i].gl_pathc : 1;
	  donesomething = rc[i] == 0;
	}
      else
	{
	  rc[i] = -1; /* glob not called */
	  ++newac;
	}
    }
  
  if (donesomething)
    {
      char **newav = (char **)malloc (sizeof (char **) * (newac + 1));
      int d = 0;
      for (i = 0; i < ac; i++)
	{
	  if (rc[i] == 0)
	    {
	      debug_printf ("`%s' converted to:", av[i]);
	      for (int j = 0; j < globs[i].gl_pathc; j++)
		{
		  debug_printf (" %s", globs[i].gl_pathv[j]);
		  newav[d++] = globs[i].gl_pathv[j];
		}
	      debug_printf ("\n");
	    }
	  else
	    {
	      debug_printf ("`%s' not converted, glob rc = 0x%x\n",
			    av[i], rc[i]);
	      newav[d++] = av[i];
	    }
	}
      newav[d] = 0;
      *acp = newac;
      *avp = newav;
    }
}

/*
 * Replaces -@file in the command line with the contents of the file.
 * There may be multiple -@file's in a single command line
 * A \-@file is replaced with -@file so that echo \-@foo would print
 * -@foo and not the contents of foo.
 *
 * NB: *cl must have been malloc'd so that this function can use realloc.
 *
 */
static void
insert_files (char **cl)
{
# define symbol_string "@"
# define symbol_prefix "-"
# define symbol_prefix_char '-'
  static const char *short_symbol = symbol_string; /* "@" */
  static const int short_symbol_size = 1;
  static const char *long_symbol = symbol_prefix symbol_string; /* "-@ */
  static const int long_symbol_size = 1 + short_symbol_size;
  const char *symbol;
  int symbol_size;
  char *cp;
  DWORD i = 0;
  size_t cl_len = strlen (*cl);

  /* The first argument is the program name and cannot not be -@ so we
     skip the first couple of characters we can use cp[-1] later on. */
  cp = *cl + 2;

  while ((cp = strstr (cp, short_symbol)))
    {
      /* test if this is the short_symbol or the long_symbol */
      if (cp[-1] == symbol_prefix_char)
        {
          symbol = long_symbol;
          symbol_size = long_symbol_size;
          cp--;
        }
      else
        {
          symbol = short_symbol;
          symbol_size = short_symbol_size;
        }
      /* If -@ is precedented by a \ than remove the \ character. */
      if (cp[-1] == '\\') 
        {
          i = cp - *cl;
          /* The + 1 is so that the terminating \0 is moved as well. */
          memmove (cp - 1, cp, cl_len - i + 1);
          cl_len--;
          cp++;
        }
      else if (cp[-1] == ' ' || cp[-1] == '\t')
        {
          /* legitimate -@file need to insert file */
          HANDLE f;
          char *name;
          char *end;
          DWORD size;
          size_t name_len;

          name = cp + symbol_size;
          end = name;

          while ((*end != ' ') && (*end != '\t') && (*end != 0))
            end++;
          *end = '\0';
          name_len = end - name;
 
          f = CreateFile (name,
                          GENERIC_READ,          /* open for reading   */
                          FILE_SHARE_READ,       /* share for reading  */
                          NULL,                  /* no security        */
                          OPEN_EXISTING,         /* existing file only */ 
                          FILE_ATTRIBUTE_NORMAL, /* normal file        */
                          NULL);                 /* no attr. template  */
 
          if (f == INVALID_HANDLE_VALUE) 
            { 
              small_printf ("%sfile: file access ERROR - %s\n", symbol, name);
              ExitProcess (1);
            } 
          else 
            {
              /* This only supports files up to about 4 billion bytes in
                 size.  I am making the bold assumption that this is big
                 enough for this feature */
              size = GetFileSize (f, NULL);
              if (size == 0xFFFFFFFF)
                {
                  small_printf ("%sfile: file access ERROR - %s\n", 
                                symbol, name);
                  ExitProcess (1);
                }
              else
                {
                  void *tmp;
                  int new_size;
                  int cp_index = cp - *cl;
                  new_size =  cl_len - name_len + size;
                  tmp = realloc (*cl, new_size);
                  if (tmp)
                    {
                      /* realloc passed as it should */
                      DWORD rf_read;
                      BOOL rf_result;

                      *cl = (char *) tmp;
                      cp = *cl + cp_index;
                      name = cp + symbol_size;
                      end = name + name_len;
                      *end = ' ';
                      /* Move end of command line to make room for file.
                         The -s after size is to overwrite the -@ chars */
                      memmove (&cp[size], end, cl_len - (end - *cl));
                      rf_result = ReadFile (f, cp, size, &rf_read, NULL);
                      if (rf_result && (rf_read == size))
                        {
                          /* Convert all cr and lf to spaces. */
                          for (i= 0; i < size; i++) 
                            {
                              if ((cp[i] == '\n') || (cp[i] == '\r'))
                                {
                                  cp[i] = ' ';
                                }
                            }
                          cl_len += size - name_len - symbol_size;
                          (*cl)[cl_len] = '\0';
                          /* (*cl)[cl_len+1] = '\0'; */
                        }
                      else
                        {
                          /* ReadFile failed or did not read complete file. */
                          small_printf ("%sfile: read ERROR - %s\n", 
                                        symbol, name);
                          ExitProcess (1);
                        }
                    }
                  else
                    {
                      /* realloc failed */
                      small_printf ("%sfile : out of memory ERROR\n", symbol);
                      ExitProcess (1);
                    }
                }
            }
        }
      else {
        /* The -@file has a leter before the - e.g. a-@file
           so we will skip over this illigimate useage  */
        cp += 3; /* skip ahead */
      }
    }
}

/* Utility to probe the stack.  */

static void
recur (void *p, void *)
{
  char b[1000];
  int x;

  if (&x > p)
    recur (p, b);
}

/* sanity and sync check */
void
check_sanity_and_sync (per_process *p)
{
  /* Sanity check to make sure developers didn't change the per_process    */
  /* struct without updating SIZEOF_PER_PROCESS [it makes them think twice */
  /* about changing it].                                                   */
  if (sizeof (per_process) != SIZEOF_PER_PROCESS)
    {
      small_printf ("per_process sanity check failed\n");
      ExitProcess (1);
    }
  
  /* Make sure that the app and the dll are in sync.
    magic_biscuit != 0 if using the old style version numbering scheme.  */
  if (p->magic_biscuit != SIZEOF_PER_PROCESS
      || p->version_major != cygwin_dll_version_major)
    {
      small_printf ("CYGWIN DLL and APP out of sync, you'll have to relink, sorry.\n");
      small_printf ("magic numbers (app %d, dll %d)\n",
		  p->magic_biscuit, 0);
      if (p->magic_biscuit == 0)
        small_printf ("cygwin version numbers (app %d.%d, dll %d.%d)\n",
		    p->version_major, p->version_minor,
		    cygwin_dll_version_major, cygwin_dll_version_minor);
      ExitProcess (1);
    }
}

/* Take over from libc's crt0.o and start the application.  */
static void
dll_crt0_1 (per_process *uptr)
{
  int argc;
  char **argv;
  int i;
  /* According to onno@stack.urc.tue.nl, the exception handler record must
     be on the stack.  */
  /* FIXME: Verify forked children get their exception handler set up ok.  */
  struct exception_list cygwin_except_entry;

  check_sanity_and_sync (uptr);

  /* Set the local copy of the pointer into the user space.  */
  user_data = uptr;

  /* Nasty static stuff needed by newlib  - point to a local copy of
     the reent stuff. 
     Note: this MUST be done here (before the forkee code) as the
     fork copy code doesn't copy the data in libccrt0.cc (that's why we
     pass in the per_process struct into the .dll from libccrt0).
  */

  *(user_data->impure_ptr_ptr) = &reent_data;
  _impure_ptr = &reent_data;

#if 0
  small_printf ("&reent_data = %x, &_impure_ptr = %x, _impure_ptr = %x\n",
		&reent_data, &_impure_ptr, _impure_ptr);
#endif

  SetFileApisToOEM ();

  /* Initialize the host dependent constants object */
  host_dependent.init ();

  /* Initialize the cygwin32 subsystem if this is the first process,
     or attach to the shared data structure if it's already running.  */
  shared_init ();

  if (user_data->forkee)
    myself = cygwin_shared->p[user_data->forkee];

  /* Initialize events */
  events_init ();

  /* Initialize SIGSEGV handling, etc.
     The exception handler references data in the shared area, so this can't
     be done until after shared_init.  */
  /* FIXME: For the forkee, our stacked copy of cygwin_except_entry will
     get overwritten, but presumably with the same data.  Not sure what
     happens in the forkee if a SIGSEGV happens before now.  */
  init_exceptions (&cygwin_except_entry);

  /* Initialize the heap.  */
  heap_init ();

#ifndef NOSTRACE
  /* Initialize strace early.  May also be initialized by later
   * processing in environ_init.
   */
  if (!user_data->forkee)
    pinfo_init ();		/* Initialize our process table entry.  */
#endif

  /* Initialize the fork mechanism.  */
  fork_init ();

  if (!old_title && GetConsoleTitle (title_buf, TITLESIZE))
      old_title = title_buf;

  /* If this is a fork, then do that now.  All the other stuff will
     have been set up by the task that forked us.  */

  if (user_data->forkee)
    {
      /* When a forked child starts, its stack is quite small.
	 However, when it longjmp's, the stack becomes that of its parent
	 which is arbitrarily large.  This can cause the child to crash so
	 probe the stack out.  */
      /* FIXME: Fix hardcoding of size after migration.  */
      recur ((char *) &i - 0x20000, 0);

      debug_printf ("child about to longjmp\n");
      dump_jmp_buf (myself->restore);

      /* The corresponding setjmp is in fork.cc.  */
      longjmp (myself->restore, user_data->forkee);
    }

  if (!old_title && GetConsoleTitle (title_buf, TITLESIZE))
      old_title = title_buf;

  (void) SetErrorMode (SEM_FAILCRITICALERRORS);

  /* Nasty static stuff needed by newlib - initialize it. 
     Note that impure_ptr has already been set up to point to this above
     NB. This *MUST* be done here, just after the forkee code as some
     of the calls below (eg. uinfo_init) do stdio calls - this area must
     be set to zero before then.
  */
  memset (&reent_data, 0, sizeof (reent_data));
  reent_data._errno = 0;
  reent_data._stdin =  reent_data.__sf + 0;
  reent_data._stdout = reent_data.__sf + 1;
  reent_data._stderr = reent_data.__sf + 2;

  /* Perform various startup sanity checks.  */
  checkout ();

  char *line = GetCommandLineA ();
#ifdef NEEDOEM
  CharToOem (line, line);
#endif

  /* Set new console title if appropriate */

  if (display_title)
    {
      char *cp = line;
      if (strip_title_path)
	for (char *ptr = cp; *ptr && *ptr != ' '; ptr++)
	  if (*ptr == '/' || *ptr == '\\')
	    cp = ptr+1;
      set_console_title (cp);
    }

  /* Initialize signal/subprocess handling */
  sigproc_init ();

  /* connect to tty */
  tty_init ();

  /* Initialize the file descriptor table.  */
  hmap_init ();

  /* Initialize uid, gid.  */
  uinfo_init ();

  /* Initialize Winsock if parent process used sockets since we
     might be inheriting socket fds as a result of a fork or exec. */
  if (ISSTATE(myself, PID_SOCKETS_USED))
    winsock_init ();

  syscall_printf ("Application CYGWIN version: %d.%d\n",
		  user_data->version_major, user_data->version_minor);
  syscall_printf ("CYGWIN DLL version        : %d.%d\n",
		  cygwin_dll_version_major, cygwin_dll_version_minor);


  /* Scan the command line and build argv.  */
  int len = strlen (line);
  char *mine = (char *) malloc (len + 2);
 
  memcpy (mine, line, len);
  mine[len] = 0;
  mine[len+1] = 0;

  /* replace -@file with file contents */
  insert_files (&mine);

  len = strlen (mine);

  argc = compute_argc (mine);
  argv = (char **) alloca ((argc + 1) * sizeof (char *));
  build_argv (mine, argv, len);

  /* Expand *.c, etc.  */
  if (NOTSTATE(myself, PID_CYGPARENT) && allow_glob)
    globify (&argc, &argv);

  /* Convert argv[0] to posix rules if we're using them and it's currently
     blatantly win32 style.  */
  if (cygwin_shared->mount.posix_path_p ()
      && (strchr (argv[0], ':') || strchr (argv[0], '\\')))
    {
      char *new_argv0 = (char *) alloca (MAX_PATH);
      conv_to_posix_path (argv[0], new_argv0);
      argv[0] = new_argv0;
    }

  getprogname (argv[0]);
  /* Set up __progname for getopt error call */
  __progname = argv[0];

#ifndef NOSTRACE
  if (myself->strace_mask & (_STRACE_STARTUP | _STRACE_ALL))
    {
      for (i = 0; i < argc; ++i)
	strace_printf ("argv[%d] = %s\n", i, argv[i]);
      for (i = 0; (*user_data->envptr)[i]; ++i)
	strace_printf ("envp[%d] = %x %s\n", i,
                         (*user_data->envptr)[i], (*user_data->envptr)[i]);
    }
#endif

  /* call init of loaded dlls */
  DllList::the().initAll();
  
  syscall_printf ("CYGWIN Release %d.%d, compiled %s %s\n",
		  cygwin_dll_version_major, cygwin_dll_version_minor,
		  __DATE__, __TIME__);

  exit (user_data->main (argc, argv, *user_data->envptr));
}

/* Wrap the real one, otherwise gdb gets confused about
   two symbols with the same name, but different addresses.

   UPTR is a pointer to global data that lives on the libc side of the
   line [if one distinguishes the application from the dll].  */

void
dll_crt0 (per_process *uptr)
{
  extern void (*__CTOR_LIST__) (void);
  int i;
  void (**pfunc)() = &__CTOR_LIST__;
  
  for (i = 1; pfunc[i]; i++)
    {
      (pfunc[i]) ();
    }

  dll_crt0_1 (uptr);
}

void
__do_global_dtors (void)
{
  int i;
  void (**pfunc)() = user_data->dtors;

  /* Run dtors backwards, so skip the first entry and find how many
     there are, then run them.  */
  
  if (pfunc) 
    {
      for (i = 1; pfunc[i]; i++) 
	;
      for (i = i - 1; i > 0; i--)
	{
	  (pfunc[i]) ();
	}
    }
}

void
__do_global_ctors (void)
{
  int i;
  void (**pfunc)() = user_data->ctors;
  
  for (i = 1; pfunc[i]; i++)
    {
      (pfunc[i]) ();
    }
  atexit (__do_global_dtors);
}

void
__main (void)
{
  if (! user_data->run_ctors_p)
    {
      user_data->run_ctors_p = 1;
      __do_global_ctors ();
    }
}

void
do_exit (int status)
{
  BOOL cleanup_pinfo;
  UINT n = (UINT) status;
  extern HANDLE hExeced;

  syscall_printf ("do_exit (%d)\n", n);

  if (n & EXIT_NOCLOSEALL)
    n &= ~EXIT_NOCLOSEALL;
  else
    close_all_files ();

  sigproc_terminate ();

  if (n & EXIT_REPARENTING)
    {
      n &= ~EXIT_REPARENTING;
      cleanup_pinfo = FALSE;
    }
  else
    {
      myself->stopsig = 0;
      cleanup_pinfo = TRUE;

      /* restore console title */
      if (old_title && display_title)
	set_console_title (old_title);

      /* Kill orphaned children on group leader exit */
      if (myself->pid == myself->pgid)
	{
	  sigproc_printf ("%d == pgrp %d, send SIG{HUP,CONT} to stopped children\n",
			  myself->pid, myself->pgid);
	  kill_pgrp (myself->pgid, -SIGHUP);
	}

      /* Kill the foreground process group on session leader exit */
      if (getpgrp () > 0 && myself->pid == myself->sid && myself->ctty != -1)
	{
	  tty *ttyp = cygwin_shared->t.gettty (myself->ctty);
	  sigproc_printf ("%d == sid %d, send SIGHUP to children\n",
			  myself->pid, myself->sid);

	  if (ttyp->getsid () == myself->sid)
	    kill (-ttyp->getpgid (), SIGHUP);
	}
      tty_terminate ();
    }

  window_terminate ();
  fill_rusage (&myself->rusage_self, GetCurrentProcess ());

  shared_terminate ();
  events_terminate ();		// Does not close pinfo_mutex

  if (hExeced)
    {
      debug_printf ("Killing(%d) non-cygwin process, handle %p, wpid = %d\n",
		    n, hExeced, myself->dwSpawnedProcessId);
      TerminateProcess (hExeced, n);
      CloseHandle (hExeced);
    }

#ifndef NOSTRACE
  if (strace() & _STRACE_EXITDUMP)
    {
      sigproc_printf ("calling ExitProcess %d\n", n);
      strace_dump ();
    }
#endif
  if (cleanup_pinfo)
    myself->record_death ();	// Locks pinfo mutex
  else
    sigproc_printf ("not cleanup_pinfo\n");
  sigproc_printf ("calling ExitProcess %d\n", n);
  ExitProcess (n);		// Terminate process and release mutex
}

void _exit (int n)
{
  do_exit ((DWORD)n & 0xffff);
}

void
api_fatal (const char *msg)
{
  small_printf ("cygwin: %s\n", msg);
  /* We are going down without mercy - make sure we reset
     our process_state. */
  if ((user_data != 0) && (myself != 0))
    {
    sigproc_terminate ();
    myself->record_death_nolock ();
    }
  ExitProcess (1);
}

#ifdef OLDWAY
/* This was needed to terminate the list of import stuff */
asm (".section .idata$3\n" ".long 0,0,0,0,0");
#endif

#if _PPC_
__chkstk () { printf ("alloca! called\n"); return 0;}
_alloca () { printf ("alloca! called\n"); return 0;}

asm (".global dll_entry@12");
asm ("dll_entry@12:");
#endif
