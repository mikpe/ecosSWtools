/* shared.cc: shared data area support.

   Copyright 1996, 1997, 1998 Cygnus Solutions.

This file is part of Cygwin32.

This software is a copyrighted work licensed under the terms of the
Cygwin32 license.  Please consult the file "CYGWIN32_LICENSE" for
details. */

#include <unistd.h>
#include <stdio.h>
#include "winsup.h"
#include "registry.h"

shared_info NO_COPY *cygwin_shared;

/* The handle of the shared data area.  */
static HANDLE NO_COPY cygwin_shared_h;

extern
char *
shared_name (char *str, int num)
{
  static NO_COPY char buf[MAX_PATH];
  extern char cygwin_dll_id[];

  sprintf (buf, "%s.%s.%d", cygwin_dll_id, str, num);
  return buf;
}

/* Open the shared memory map.  */
static void
open_shared_file_map ()
{
  char *mapname = shared_name ("shared", 0);

  cygwin_shared_h = OpenFileMappingA (FILE_MAP_READ | FILE_MAP_WRITE,
				                          FALSE, mapname);

  if (!cygwin_shared_h)
    {
      SECURITY_ATTRIBUTES sa;
      sa.nLength = sizeof (SECURITY_ATTRIBUTES);
      sa.bInheritHandle = 0;
      sa.lpSecurityDescriptor = get_null_sd ();

      cygwin_shared_h = CreateFileMappingA ((HANDLE) 0xffffffff,
			      &sa,
			      PAGE_READWRITE,
			      0,
			      sizeof (*cygwin_shared),
			      mapname);

    }

  if (!cygwin_shared_h)
    {
      system_printf ("open_shared_file_map: CreateFileMappingA\n");
      api_fatal ("terminating");
    }

  cygwin_shared = (shared_info *) MapViewOfFileEx (cygwin_shared_h,
                                       FILE_MAP_READ | FILE_MAP_WRITE,
				       0, 0, 0, (void *) 0xa000000);

  if (!cygwin_shared)
    { 
      /* Probably win95, so try without specifying the address.  */
      cygwin_shared = (shared_info *) MapViewOfFileEx (cygwin_shared_h,
                                       FILE_MAP_READ|FILE_MAP_WRITE,
                                       0,0,0,0);
    }

  if (!cygwin_shared)
    {
      system_printf ("open_shared_file_map: MapViewOfFileEx\n");
      api_fatal ("terminating");
    }

  debug_printf ("cygwin_shared = %p, h = %d\n", cygwin_shared,
		                                      cygwin_shared_h);

  /* FIXME: I couldn't find anywhere in the documentation a note about
     whether the memory is initialized to zero.  The code assumes it does
     and since this part seems to be working, we'll leave it as is.  */
}

void
shared_info::initialize ()
{
  /* Ya, Win32 provides a way for a dll to watch when it's first loaded.
     We may eventually want to use it but for now we have this.  */
  if (inited)
    return;

  /* Initialize the mount table.  */
  mount.init ();

  /* Initialize the process table.  */
  p.init ();

  /* Initialize the queue of deleted files.  */
  delqueue.init ();

  /* Initialize tty table.  */
  t.init ();

  /* Fetch misc. registry entries.  */

  reg_session reg;

  /* Note that reserving a huge amount of heap space does not result in
  swapping since we are not committing it. */
  /* FIXME: We should not be restricted to a fixed size heap no matter
  what the fixed size is! */

  heap_chunk_in_mb = reg.get_key ().get_int ("heap_chunk_in_mb", 128);
  if (heap_chunk_in_mb < 4)
    {
      heap_chunk_in_mb = 4;
      reg.get_key ().set_int ("heap_chunk_in_mb", heap_chunk_in_mb);
    }

  inited = 1;
}

void
shared_init ()
{
  open_shared_file_map ();

  cygwin_shared->initialize ();
}

void
shared_terminate ()
{
  if (cygwin_shared_h)
    CloseHandle (cygwin_shared_h);
}

int
shared_info::heap_chunk_size ()
{
  return heap_chunk_in_mb << 20;
}

/* For apps that wish to access the shared data.  */

shared_info *
cygwin32_getshared ()
{
  return cygwin_shared;
}

/*
 * Function to return a common SECURITY_DESCRIPTOR * that
 * allows all access.
 */

static NO_COPY SECURITY_DESCRIPTOR *null_sdp = 0;

SECURITY_DESCRIPTOR *get_null_sd ()
{
  static NO_COPY SECURITY_DESCRIPTOR sd;

  if (null_sdp == 0)
    {
      InitializeSecurityDescriptor (&sd, SECURITY_DESCRIPTOR_REVISION);
      SetSecurityDescriptorDacl (&sd, TRUE, 0, FALSE);
      null_sdp = &sd;
    }
  return null_sdp;
}
