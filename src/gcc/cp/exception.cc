// Functions for Exception Support for -*- C++ -*-
// Copyright (C) 1994, 1995, 1996 Free Software Foundation

// This file is part of GNU CC.

// GNU CC is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or (at your option)
// any later version.

// GNU CC is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with GNU CC; see the file COPYING.  If not, write to
// the Free Software Foundation, 59 Temple Place - Suite 330,
// Boston, MA 02111-1307, USA. 

// As a special exception, if you link this library with other files,
// some of which are compiled with GCC, to produce an executable,
// this library does not by itself cause the resulting executable
// to be covered by the GNU General Public License.
// This exception does not however invalidate any other reasons why
// the executable file might be covered by the GNU General Public License.

#pragma implementation "exception"

#ifndef __EMBEDDED_CXX__
#include "typeinfo"
#include "exception"
#include <stddef.h>

/* Define terminate, unexpected, set_terminate, set_unexpected as
   well as the default terminate func and default unexpected func.  */

extern terminate_handler __terminate_func __attribute__((__noreturn__));

void
terminate ()
{
  __terminate_func ();
}

void
__default_unexpected ()
{
  terminate ();
}

static unexpected_handler __unexpected_func __attribute__((__noreturn__))
  = __default_unexpected;

terminate_handler
set_terminate (terminate_handler func)
{
  terminate_handler old = __terminate_func;

  __terminate_func = func;
  return old;
}

unexpected_handler
set_unexpected (unexpected_handler func)
{
  unexpected_handler old = __unexpected_func;

  __unexpected_func = func;
  return old;
}

void
unexpected ()
{
  __unexpected_func ();
}

/* C++-specific state about the current exception.
   This must match init_exception_processing().

   Note that handlers and caught are not redundant; when rethrown, an
   exception can have multiple active handlers and still be considered
   uncaught.  */

struct cp_eh_info
{
  void *value;
  void *type;
  void (*cleanup)(void *, int);
  bool caught;
  cp_eh_info *next;
  long handlers;
};

/* Language-specific EH info pointer, defined in libgcc2. */

extern "C" cp_eh_info **__get_eh_info (); 	// actually void **

/* Is P the type_info node for a pointer of some kind?  */

extern bool __is_pointer (void *);

/* Compiler hook to return a pointer to the info for the current exception.
   Used by get_eh_info ().  */

extern "C" cp_eh_info *
__cp_exception_info (void)
{
  return *__get_eh_info ();
}

/* Allocate a buffer for a cp_eh_info and an exception object of size SIZE,
   and return a pointer to the beginning of the object's space.  */

extern "C" void * malloc (size_t);
extern "C" void *
__eh_alloc (size_t size)
{
  void *p = malloc (size);
  if (p == 0)
    terminate ();
  return p;
}

/* Free the memory for an cp_eh_info and associated exception, given
   a pointer to the cp_eh_info.  */

extern "C" void free (void *);
extern "C" void
__eh_free (void *p)
{
  free (p);
}

/* Compiler hook to push a new exception onto the stack.
   Used by expand_throw().  */

extern "C" void
__cp_push_exception (void *value, void *type, void (*cleanup)(void *, int))
{
  cp_eh_info *p = (cp_eh_info *) __eh_alloc (sizeof (cp_eh_info));

  p->value = value;
  p->type = type;
  p->cleanup = cleanup;
  p->handlers = 0;
  p->caught = false;

  cp_eh_info **q = __get_eh_info ();

  p->next = *q;
  *q = p;
}

/* Compiler hook to pop an exception that has been finalized.  Used by
   push_eh_cleanup().  P is the info for the exception caught by the
   current catch block.  */

extern "C" void
__cp_pop_exception (cp_eh_info *p)
{
  cp_eh_info **q = __get_eh_info ();

  --p->handlers;

  /* Don't really pop if there are still active handlers for our exception,
     or if our exception is being rethrown (i.e. if the active exception is
     our exception and it is uncaught).  */
  if (p->handlers != 0
      || (p == *q && !p->caught))
    return;

  for (; *q; q = &((*q)->next))
    if (*q == p)
      break;

  if (! *q)
    terminate ();

  *q = p->next;

  if (p->cleanup)
    /* 2 is a magic value for destructors; see build_delete().  */
    p->cleanup (p->value, 2);

  if (! __is_pointer (p->type))
    __eh_free (p->value);

  __eh_free (p);
}

extern "C" void
__uncatch_exception (void)
{
  cp_eh_info *p = __cp_exception_info ();
  if (p == 0)
    terminate ();
  p->caught = false;
}

/* As per [except.unexpected]:
   If an exception is thrown, we check it against the spec.  If it doesn't
   match, we call unexpected ().  If unexpected () throws, we check that
   exception against the spec.  If it doesn't match, if the spec allows
   bad_exception we throw that; otherwise we call terminate ().

   The compiler treats an exception spec as a try block with a generic
   handler that just calls this function with a list of the allowed
   exception types, so we have an active exception that can be rethrown.

   This function does not return.  */   

extern "C" void
__check_eh_spec (int n, const void **spec)
{
  cp_eh_info *p = __cp_exception_info ();

  for (int i = 0; i < n; ++i)
    {
      if (__throw_type_match_rtti (spec[i], p->type, p->value))
	throw;
    }

  try
    {
      unexpected ();
    }
  catch (...)
    {
      // __exception_info is an artificial var pushed into each catch block.
      if (p != __exception_info)
	{
	  p = __exception_info;
	  for (int i = 0; i < n; ++i)
	    {
	      if (__throw_type_match_rtti (spec[i], p->type, p->value))
		throw;
	    }
	}

      const type_info &bad_exc = typeid (bad_exception);
      for (int i = 0; i < n; ++i)
	{
	  if (__throw_type_match_rtti (spec[i], &bad_exc, p->value))
	    throw bad_exception ();
	}

      terminate ();
    }
}

extern "C" void
__throw_bad_cast (void)
{
  throw bad_cast ();
}

extern "C" void
__throw_bad_typeid (void)
{
  throw bad_typeid ();
}

/* Has the current exception been caught?  */

bool
uncaught_exception ()
{
  cp_eh_info *p = __cp_exception_info ();
  return p && ! p->caught;
}

const char * exception::
what () const
{
  return typeid (*this).name ();
}
#endif
