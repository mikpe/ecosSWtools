/* pinfo.cc: process table support

   Copyright 1996, 1997, 1998 Cygnus Solutions.

This file is part of Cygwin32.

This software is a copyrighted work licensed under the terms of the
Cygwin32 license.  Please consult the file "CYGWIN32_LICENSE" for
details. */

#include <stdlib.h>
#include <time.h>
#include "winsup.h"

/* The first pid used; also the lowest value allowed. */
#define PBASE 1000

pinfo NO_COPY *myself = NULL;

/* Initialize the process table.
   This is done once when the dll is first loaded.  */

void
pinfo_list::init (void)
{
  next_pid = PBASE;	/* Next pid to try to allocate.  */

  /* We assume the shared data area is already initialized to zeros.
     Note that SIG_DFL is zero.  */
}

/* Initialize the process table entry for the current task.
   This is not called for fork'd tasks, only exec'd ones.  */
void
pinfo_init (void)
{
  DWORD thisProcessId = GetCurrentProcessId ();

  /*
   * We need to lock the process table here to be safe from
   * manipulation by a fork in another process.
   */

  myself = NULL;

  /* Lookup dwSpawnedProcessId in table.  */
  for (int i = 0; i < cygwin_shared->p.size (); i++)
    if (cygwin_shared->p.vec[i].dwSpawnedProcessId == thisProcessId &&
	cygwin_shared->p.vec[i].process_state != PID_NOT_IN_USE)
      {
	myself = cygwin_shared->p.vec + i;
	myself->dwSpawnedProcessId = 0;
	break;
      }

  if (myself)
    {
      /* The process was execed.  Reuse entry from the original
	 owner of this pid.
       */
      environ_init ();	  /* Needs user_data->self but affects calls below */

      /* spawn has already set up a pid structure for us so we'll use that */
      
      (void) lock_pinfo_for_update (INFINITE);
      myself->process_state |= PID_CYGPARENT;
      myself->start_time = time (NULL); /* Register our starting time. */

      /*
       * The value in myself->hmap.vec will be garbage as it
       * will point into our parent's address space. Allocate space
       * for getdtablesize() file descriptors and copy from
       * STARTUPINFO structure.
       */
      STARTUPINFO si;

      GetStartupInfo (&si);
      if (si.cbReserved2 == 0 || si.lpReserved2 == NULL)
	api_fatal ("Can't inherit fd table\n");
      else
	{
	  LPBYTE b =
	    myself->hmap.de_linearize_fd_array (si.lpReserved2 + sizeof (int));
	  if (b && *b)
	    old_title = (char *)b;
	}

      unlock_pinfo ();
      debug_printf ("new self->hmap.vec = %x\n", myself->hmap.vec);
    }
  else
    {
      /* Invent our own pid.  */

      (void) lock_pinfo_for_update (INFINITE);
      if (!(myself = cygwin_shared->p.allocate_pid ()))
	api_fatal ("No more processes");

      myself->start_time = time (NULL); /* Register our starting time. */

      myself->ppid = myself->pid;
      myself->sid  = myself->pid;
      myself->pgid = 0;
      myself->ctty = -1;
      myself->uid = USHRT_MAX;
      unlock_pinfo ();

      environ_init ();		/* call after myself has been set up */
    }

  debug_printf ("pid == %d, pgid == %d\n", myself->pid, myself->pgid);
}

/* [] operator.  This is the mechanism for table lookups.  */
/* Returns the index into the pinfo_list table for pid arg */

pinfo *
pinfo_list::operator [] (pid_t pid)
{
  pinfo *p = vec + (pid % size ());

  if (p->process_state == PID_NOT_IN_USE)
    return NULL;
  else
    return p;
}

pinfo *
procinfo (int pid)
{
  return cygwin_shared->p[pid];
}

#ifdef DEBUGGING
/*
 * Code to lock/unlock the process table.
 */

int
lpfu (char *func, int ln, DWORD timeout)
{
  int rc;
  DWORD t;

  debug_printf ("timeout %d, pinfo_mutex = %p\n", timeout, pinfo_mutex);
  t = (timeout == INFINITE) ? 60000 : timeout;
  SetLastError(0);
  while ((rc = WaitForSingleObject (pinfo_mutex, t)) != WAIT_OBJECT_0)
    {
      if (rc == WAIT_ABANDONED_0)
	break;
      system_printf ("lock_pinfo_for_update: %s:%d having problems getting lock\n", 
		     func, ln);
      system_printf ("lock_pinfo_for_update: *** %src = %d, err = %d\n",
		     cygwin_shared->p.lock_info, rc, GetLastError());
      if (t == timeout)
	break;
     }

  __small_sprintf (cygwin_shared->p.lock_info, "%s(%d), pid %d ", func, ln,
		   (u && myself) ? myself->pid : -1);
  return rc;
}

void
unlock_pinfo (void)
{

  debug_printf ("handle %d\n", pinfo_mutex);

  if (!cygwin_shared->p.lock_info[0])
    system_printf ("unlock_pinfo: lock_info not set?\n");
  else
    strcat (cygwin_shared->p.lock_info, " unlocked ");
  ReleaseMutex (pinfo_mutex);
}
#else
/*
 * Code to lock/unlock the process table.
 */

int
lock_pinfo_for_update (DWORD timeout)
{
  int rc;
  DWORD t;

  debug_printf ("timeout %d, pinfo_mutex = %p\n", timeout, pinfo_mutex);
  t = (timeout == INFINITE) ? 60000 : timeout;
  SetLastError(0);
  while ((rc = WaitForSingleObject (pinfo_mutex, t)) != WAIT_OBJECT_0)
    {
      if (rc == WAIT_ABANDONED_0)
	break;
      system_printf ("lock_pinfo_for_update: rc = %d, err = %d\n", rc,
		     GetLastError());
      if (t == timeout)
	break;
     }

  return rc;
}

void
unlock_pinfo (void)
{

  debug_printf ("handle %d\n", pinfo_mutex);

  ReleaseMutex (pinfo_mutex);
}
#endif


/* Allocate a process table entry by finding an empty slot in the
   fixed-size process table.  We could use a linked list, but this
   would probably be too slow.

   Try to allocate next_pid, incrementing next_pid and trying again
   up to size() times at which point we reach the conclusion that
   table is full.  Eventually at this point we would grow the table
   by size() and start over.  If we find a pid to use, 

   If all else fails, sweep through the loop looking for processes that
   may have died abnormally without registering themselves as "dead".
   Clear out these pinfo structures.  Then scan the table again.

   Note that the process table is in the shared data space and thus
   is susceptible to corruption.  The amount of time spent scanning the
   table is presumably quite small compared with the total time to
   create a process.
*/

pinfo *
pinfo_list::allocate_pid (void)
{

  pinfo *newp;

  for (int tries = 0; ; tries++)
    {
      for (int i = next_pid; i < (next_pid + size ()); i++)
	{
	  /* i mod size() gives place to check */
	  newp = vec + (i % size());
	  if (newp->process_state == PID_NOT_IN_USE)
	    {
	      debug_printf ("found empty slot %d for pid %d\n",
                                                   (i % size ()), i);
	      next_pid = i;
	      goto gotit;
	    }
	}

      if (tries > 0)
	break;

      /* try once to remove bogus dead processes */
      debug_printf ("clearing out deadwood\n");
      for (newp = vec; newp < vec + size(); newp++)
	proc_exists (newp);
    }

  /* The process table is full.  */
  debug_printf ("process table is full\n");

  return NULL;

gotit:
  memset (newp, 0, PINFO_ZERO);

  /* Set new pid based on the position of this element in the pinfo list */
  newp->pid = next_pid;

  /* Determine next slot to consider, wrapping if we hit the end of
   * the array.  Since allocation involves looping through size () pids,
   * don't allow next_pid to be greater than INT_MAX - size ().
   */
  if (next_pid < (INT_MAX - size ()))
    next_pid++;
  else
    next_pid = PBASE;

  /* Allocate the file descriptor table in hmap. We do this so that
   * the number of fd's per process can be changed.
   */
  if (!(newp->hmap.vec = new hinfo [getdtablesize ()]))
    api_fatal ("pinfo_list::allocate_pid() - out of memory\n");

  newp->hmap.clearout ();
  newp->process_state = PID_IN_USE;
  debug_printf ("pid %d, state %x\n", newp->pid, newp->process_state);
  return newp;
}

void
pinfo::record_death_nolock (void)
{
  if (myself->dwProcessId == GetCurrentProcessId () && !alive_parent (myself))
    {
      process_state = PID_NOT_IN_USE;
      hProcess = NULL;
    }
}

void
pinfo::record_death (void)
{
  (void) lock_pinfo_for_update (INFINITE);

  record_death_nolock ();

//unlock_pinfo ();	// Lock until ExitProcess or suffer a race
}

void
pinfo::init_from_exec (void)
{
  /* Close all the close_on exec things and duplicate the rest into
     the child's space  */

  /* memset (&reent_data, 0, sizeof (reent_data));*/
  debug_printf ("in init_from_exec\n");

  hmap.dup_for_exec ();
}

/* This function is exported from the DLL.  Programs can call it to
   convert a Windows process ID into a cygwin process ID.  */

extern "C" pid_t
cygwin32_winpid_to_pid (int winpid)
{
  for (int i = 0; i < cygwin_shared->p.size (); i++)
    {
      pinfo *p = &cygwin_shared->p.vec[i];

      if (p->process_state == PID_NOT_IN_USE)
        continue;

      /* FIXME: signed vs unsigned comparison: winpid can be < 0 !!! */
      if (p->dwProcessId == winpid)
	return p->pid;
    }

  set_errno (ESRCH);
  return (pid_t) -1;
}
