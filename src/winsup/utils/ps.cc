/* ps.cc

   Copyright 1996, 1997, 1998 Cygnus Solutions.

This file is part of Cygwin32.

This software is a copyrighted work licensed under the terms of the
Cygwin32 license.  Please consult the file "CYGWIN32_LICENSE" for
details. */

#include <stdio.h>
#include <time.h>
#include <string.h>
#include "winsup.h"

static char *
start_time (pinfo *child)
{
  time_t st = child->start_time;
  time_t t = time (NULL);
  static char stime[40] = {'\0'};
  char now[40];

  strncpy (stime, ctime (&st) + 4, 15);
  strcpy (now, ctime (&t) + 4);
  if ((t - st) < (24 * 3600))
      return stime + 7;
  stime[6] = '\0';
  return stime;
}

int
main (int argc, char *argv[])
{
  BOOL full;
  shared_info *s = cygwin32_getshared ();
  char *title =     "  %8s %8s %8s %8s %4s %3s %8s %s\n";
  char *format = "%c %8d %8d %8d %8d %4d %3d %8s %s\n";

  printf (title, "PID", "PPID", "PGID", "WINPID", "UID", "TTY",
	  "STIME", "COMMAND");

  if (argv[1] && strcmp (argv[1], "-f") == 0)
    full = TRUE;
  else
    full = FALSE;

  for (int i = 0; i < s->p.size (); i++)
    {
      pinfo *p = &s->p.vec[i];

      if (p->process_state == PID_NOT_IN_USE ||
	  (!full && kill (p->pid, 0)))
	continue;
      char status = ' ';
      if (p->process_state & PID_STOPPED)
	status = 'S';
      else if (p->process_state & PID_TTYIN)
	status = 'I';
      else if (p->process_state & PID_TTYOU)
	status = 'O';
      printf (format, status, p->pid, p->ppid, p->pgid,
	      p->dwProcessId, p->uid, p->ctty, start_time (p),
	      (p->process_state & PID_ZOMBIE) ? "<defunct>" : p->progname);
    }

  return 0;
}
