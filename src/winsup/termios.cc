/* termios.cc: termios for WIN32.

   Copyright 1996, 1997, 1998 Cygnus Solutions.

   Written by Doug Evans and Steve Chamberlain of Cygnus Support
   dje@cygnus.com, sac@cygnus.com

This file is part of Cygwin32.

This software is a copyrighted work licensed under the terms of the
Cygwin32 license.  Please consult the file "CYGWIN32_LICENSE" for
details. */

#include <sys/termios.h>
#include "winsup.h"

/* tcsendbreak: POSIX 7.2.2.1 */
extern "C"
int
tcsendbreak (int fd, int duration)
{
  int res = -1;

  if (NOT_OPEN_FD (fd))
    set_errno (EBADF);
  else if (!myself->hmap[fd].h->is_tty ())
    set_errno (ENOTTY);
  else
    res = myself->hmap[fd].h->tcsendbreak (duration);

  syscall_printf ("%d = tcsendbreak (%d, %d );\n", res, fd, duration);
  return res;
}

/* tcdrain: POSIX 7.2.2.1 */
extern "C"
int
tcdrain (int fd)
{
  int res = -1;

  termios_printf ("tcdrain\n");
  
  if (NOT_OPEN_FD (fd))
    set_errno (EBADF);
  else if (!myself->hmap[fd].h->is_tty ())
    set_errno (ENOTTY);
  else
    res = myself->hmap[fd].h->tcdrain ();

  syscall_printf ("%d = tcdrain (%d);\n", res, fd);
  return res;
}

/* tcflush: POSIX 7.2.2.1 */
extern "C"
int
tcflush (int fd, int queue)
{
  int res = -1;

  if (NOT_OPEN_FD (fd))
    set_errno (EBADF);
  else if (!myself->hmap[fd].h->is_tty ())
    set_errno (ENOTTY);
  else
    res = myself->hmap[fd].h->tcflush (queue);

  termios_printf ("%d = tcflush (%d, %d);\n", res, fd, queue);
  return res;
}

/* tcflow: POSIX 7.2.2.1 */
extern "C"
int
tcflow (int fd, int action)
{
  int res = -1;

  if (NOT_OPEN_FD (fd))
    set_errno (EBADF);
  else if (!myself->hmap[fd].h->is_tty ())
    set_errno (ENOTTY);
  else
    res = myself->hmap[fd].h->tcflow (action);

  syscall_printf ("%d = tcflow (%d, %d);\n", res, fd, action);
  return res;
}

/* tcsetattr: POSIX96 7.2.1.1 */
extern "C"
int
tcsetattr (int fd, int a, const struct termios *t)
{
  int res = -1;

  if (NOT_OPEN_FD (fd))
    set_errno (EBADF);
  else if (!myself->hmap[fd].h->is_tty ())
    set_errno (ENOTTY);
  else
    res = myself->hmap[fd].h->tcsetattr (a, t);

  termios_printf ("iflag %x oflag %x cflag %x lflag %x\n",
	t->c_iflag, t->c_oflag, t->c_cflag, t->c_lflag);
  termios_printf ("%d = tcsetattr (%d, %d, %x);\n", res, fd, a, t);
  return res;
}

/* tcgetattr: POSIX 7.2.1.1 */
extern "C"
int
tcgetattr (int fd, struct termios *t)
{
  int res = -1;
  
  if (NOT_OPEN_FD (fd))
    set_errno (EBADF);
  else if (!myself->hmap[fd].h->is_tty ())
    set_errno (ENOTTY);
  else
    res = myself->hmap[fd].h->tcgetattr (t);

  termios_printf ("%d = tcgetattr (%d, %x);\n", res, fd, t);
  return res;
}

/* tcgetpgrp: POSIX 7.2.3.1 */
extern "C"
int
tcgetpgrp (int fd)
{
  int res = -1;
  
  if (NOT_OPEN_FD (fd))
    set_errno (EBADF);
  else if (!myself->hmap[fd].h->is_tty ())
    set_errno (ENOTTY);
  else
    res = myself->hmap[fd].h->tcgetpgrp ();

  termios_printf ("%d = tcgetpgrp (%d);\n", res, fd);
  return res;
}

/* tcsetpgrp: POSIX 7.2.4.1 */
extern "C"
int
tcsetpgrp (int fd, pid_t pgid)
{
  int res = -1;
  
  if (NOT_OPEN_FD (fd))
    set_errno (EBADF);
  else if (!myself->hmap[fd].h->is_tty ())
    set_errno (ENOTTY);
  else
    res = myself->hmap[fd].h->tcsetpgrp (pgid);

  termios_printf ("%d = tcsetpgrp (%d, %x);\n", res, fd, pgid);
  return res;
}

/* NIST PCTS requires not macro-only implementation */
#undef cfgetospeed
#undef cfgetispeed
#undef cfsetospeed
#undef cfsetispeed

/* cfgetospeed: POSIX96 7.1.3.1 */
extern "C"
speed_t
cfgetospeed (struct termios *tp)
{
  return tp->c_ospeed;
}

/* cfgetispeed: POSIX96 7.1.3.1 */
extern "C"
speed_t
cfgetispeed (struct termios *tp)
{
  return tp->c_ispeed;
}

/* cfsetospeed: POSIX96 7.1.3.1 */
extern "C"
int
cfsetospeed (struct termios *tp, speed_t speed)
{
  tp->c_ospeed = speed;
  return 0;
}

/* cfsetispeed: POSIX96 7.1.3.1 */
extern "C"
int
cfsetispeed (struct termios *tp, speed_t speed)
{
  tp->c_ispeed = speed;
  return 0;
}

#if 0
static void
tdump (int)
{
  termios_printf ("fd %d rb %d wb %d len %d time %d\n",
		  fd,
		  myself->hmap[fd].r_binary,
		  myself->hmap[fd].w_binary,
		  myself->hmap[fd].vmin,
		  myself->hmap[fd].vtime);
}
#endif

#if 0
static void
ds (char *when, DCB *stty)
{
  termios_printf ("DCB state %s\n", when);
  termios_printf ("DCBlength %x\n", stty->DCBlength);
  termios_printf ("BaudRate;    %d\n", stty->BaudRate);   
  termios_printf ("fBinary;  %d\n", stty->fBinary);
  termios_printf ("fParity;   %d\n", stty->fParity);
  termios_printf ("fOutxCtsFlow	%d\n", stty->fOutxCtsFlow);
  termios_printf ("fOutxDsrFlow	%d\n", stty->fOutxDsrFlow);
  termios_printf ("fDtrControl	 %d\n", stty->fDtrControl);
  termios_printf ("fDsrSensitivity	%d\n", stty->fDsrSensitivity);
  termios_printf ("fTXContinueOnXoff	%d\n", stty->fTXContinueOnXoff);
  termios_printf ("1;      %d\n", stty->fOutX);
  termios_printf ("1;       %d\n", stty->fInX);
  termios_printf ("1; %d\n", stty->fErrorChar);
  termios_printf ("1;      %d\n", stty->fNull);
  termios_printf ("fRtsControl %d\n", stty->fRtsControl);
  termios_printf ("fAbortOnError	 %d\n", stty->fAbortOnError);
  termios_printf ("%d\n", stty->fDummy2);
  termios_printf ("res1	%d\n", stty->res1);
  termios_printf ("XonLim;          %d\n", stty->XonLim);         
  termios_printf ("XoffLim;         %d\n", stty->XoffLim);        
  termios_printf ("ByteSize;        %d\n", stty->ByteSize);       
  termios_printf ("Parity;          %d\n", stty->Parity);         
  termios_printf ("StopBits;        %d\n", stty->StopBits);       
  termios_printf ("XonChar;         %d\n", stty->XonChar);        
  termios_printf ("XoffChar;        %d\n", stty->XoffChar);       
  termios_printf ("ErrorChar;       %d\n", stty->ErrorChar);      
  termios_printf ("EofChar;         %d\n", stty->EofChar);        
  termios_printf ("EvtChar;         %d\n", stty->EvtChar);        
  termios_printf ("res2		%d\n", stty->res2);
}
#endif
