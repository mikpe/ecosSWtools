/* errno.cc: errno-related functions

   Copyright 1996, 1997, 1998 Cygnus Solutions.

This file is part of Cygwin32.

This software is a copyrighted work licensed under the terms of the
Cygwin32 license.  Please consult the file "CYGWIN32_LICENSE" for
details. */

#include <stdio.h>
#include "winsup.h"

/* Table to map Windows error codes to Errno values.  */
/* FIXME: Doing things this way is a little slow.  It's trivial to change
   this into a big case statement if necessary.  Left as is for now. */

static const struct
  {
    int w;		 /* windows version of error */
    const char *s;	 /* text of windows version */
    int e;		 /* errno version of error */
  }
errmap[] =
{
  /* FIXME: Some of these choices are arbitrary! */
  { ERROR_INVALID_FUNCTION, "ERROR_INVALID_FUNCTION", EBADRQC  },
  { ERROR_FILE_NOT_FOUND, "ERROR_FILE_NOT_FOUND", ENOENT  },
  { ERROR_PATH_NOT_FOUND, "ERROR_PATH_NOT_FOUND", ENOENT  },
  { ERROR_TOO_MANY_OPEN_FILES, "ERROR_TOO_MANY_OPEN_FILES", EMFILE  },
  { ERROR_ACCESS_DENIED, "ERROR_ACCESS_DENIED", EACCES  },
  { ERROR_INVALID_HANDLE, "ERROR_INVALID_HANDLE", EBADF  },
  { ERROR_NOT_ENOUGH_MEMORY, "ERROR_NOT_ENOUGH_MEMORY", ENOMEM  },
  { ERROR_INVALID_DATA, "ERROR_INVALID_DATA", EINVAL  },
  { ERROR_OUTOFMEMORY, "ERROR_OUTOFMEMORY", ENOMEM  },
  { ERROR_INVALID_DRIVE, "ERROR_INVALID_DRIVE", ENODEV  },
  { ERROR_NOT_SAME_DEVICE, "ERROR_NOT_SAME_DEVICE", EXDEV  },
  { ERROR_NO_MORE_FILES, "ERROR_NO_MORE_FILES", ENMFILE  },
  { ERROR_WRITE_PROTECT, "ERROR_WRITE_PROTECT", EROFS  },
  { ERROR_BAD_UNIT, "ERROR_BAD_UNIT", ENODEV  },
  { ERROR_SHARING_VIOLATION, "ERROR_SHARING_VIOLATION", EACCES  },
  { ERROR_LOCK_VIOLATION, "ERROR_LOCK_VIOLATION", EACCES  },
  { ERROR_SHARING_BUFFER_EXCEEDED, "ERROR_SHARING_BUFFER_EXCEEDED", ENOLCK  },
  { ERROR_HANDLE_EOF, "ERROR_HANDLE_EOF", ENODATA  },
  { ERROR_HANDLE_DISK_FULL, "ERROR_HANDLE_DISK_FULL", ENOSPC  },
  { ERROR_NOT_SUPPORTED, "ERROR_NOT_SUPPORTED", ENOSYS  },
  { ERROR_REM_NOT_LIST, "ERROR_REM_NOT_LIST", ENONET  },
  { ERROR_DUP_NAME, "ERROR_DUP_NAME", ENOTUNIQ  },
  { ERROR_BAD_NETPATH, "ERROR_BAD_NETPATH", ENXIO  },
  { ERROR_FILE_EXISTS, "ERROR_FILE_EXISTS", EEXIST  },
  { ERROR_CANNOT_MAKE, "ERROR_CANNOT_MAKE", EPERM  },
  { ERROR_INVALID_PARAMETER, "ERROR_INVALID_PARAMETER", EINVAL  },
  { ERROR_NO_PROC_SLOTS, "ERROR_NO_PROC_SLOTS", EAGAIN  },
  { ERROR_BROKEN_PIPE, "ERROR_BROKEN_PIPE", EPIPE  },
  { ERROR_OPEN_FAILED, "ERROR_OPEN_FAILED", EIO  },
  { ERROR_NO_MORE_SEARCH_HANDLES, "ERROR_NO_MORE_SEARCH_HANDLES", ENFILE  },
  { ERROR_CALL_NOT_IMPLEMENTED, "ERROR_CALL_NOT_IMPLEMENTED", ENOSYS  },
  { ERROR_INVALID_NAME, "ERROR_INVALID_NAME", ENOENT  },
  { ERROR_WAIT_NO_CHILDREN, "ERROR_WAIT_NO_CHILDREN", ECHILD  },
  { ERROR_CHILD_NOT_COMPLETE, "ERROR_CHILD_NOT_COMPLETE", EBUSY  },
  { ERROR_DIR_NOT_EMPTY, "ERROR_DIR_NOT_EMPTY", ENOTEMPTY  },
  { ERROR_SIGNAL_REFUSED, "ERROR_SIGNAL_REFUSED", EIO  },
  { ERROR_BAD_PATHNAME, "ERROR_BAD_PATHNAME", EINVAL  },
  { ERROR_SIGNAL_PENDING, "ERROR_SIGNAL_PENDING", EBUSY  },
  { ERROR_MAX_THRDS_REACHED, "ERROR_MAX_THRDS_REACHED", EAGAIN  },
  { ERROR_BUSY, "ERROR_BUSY", EBUSY  },
  { ERROR_ALREADY_EXISTS, "ERROR_ALREADY_EXISTS", EEXIST  },
  { ERROR_NO_SIGNAL_SENT, "ERROR_NO_SIGNAL_SENT", EIO  },
  { ERROR_FILENAME_EXCED_RANGE, "ERROR_FILENAME_EXCED_RANGE", EINVAL  },
  { ERROR_META_EXPANSION_TOO_LONG, "ERROR_META_EXPANSION_TOO_LONG", EINVAL  },
  { ERROR_INVALID_SIGNAL_NUMBER, "ERROR_INVALID_SIGNAL_NUMBER", EINVAL  },
  { ERROR_THREAD_1_INACTIVE, "ERROR_THREAD_1_INACTIVE", EINVAL  },
  { ERROR_BAD_PIPE, "ERROR_BAD_PIPE", EINVAL  },
  { ERROR_PIPE_BUSY, "ERROR_PIPE_BUSY", EBUSY  },
  { ERROR_NO_DATA, "ERROR_NO_DATA", EPIPE  },
  { ERROR_PIPE_NOT_CONNECTED, "ERROR_PIPE_NOT_CONNECTED", ECOMM  },
  { ERROR_MORE_DATA, "ERROR_MORE_DATA", EAGAIN  },
  { ERROR_DIRECTORY, "ERROR_DIRECTORY", EISDIR  },
  { ERROR_PIPE_CONNECTED, "ERROR_PIPE_CONNECTED", EBUSY  },
  { ERROR_PIPE_LISTENING, "ERROR_PIPE_LISTENING", ECOMM  },
  { ERROR_NO_TOKEN, "ERROR_NO_TOKEN", EINVAL  },
  { ERROR_PROCESS_ABORTED, "ERROR_PROCESS_ABORTED", EFAULT  },
  { ERROR_BAD_DEVICE, "ERROR_BAD_DEVICE", ENODEV  },
  { ERROR_BAD_USERNAME, "ERROR_BAD_USERNAME", EINVAL  },
  { ERROR_NOT_CONNECTED, "ERROR_NOT_CONNECTED", ENOLINK  },
  { ERROR_OPEN_FILES, "ERROR_OPEN_FILES", EAGAIN  },
  { ERROR_ACTIVE_CONNECTIONS, "ERROR_ACTIVE_CONNECTIONS", EAGAIN  },
  { ERROR_DEVICE_IN_USE, "ERROR_DEVICE_IN_USE", EAGAIN  },
  { ERROR_INVALID_AT_INTERRUPT_TIME, "ERROR_INVALID_AT_INTERRUPT_TIME", EINTR},
  { ERROR_IO_DEVICE, "ERROR_IO_DEVICE", EIO },
};

/* seterrno: standards? */
/* Set `errno' based on GetLastError (). */
extern "C"
void
seterrno (const char *file, int line)
{
  int i;
  int why = GetLastError ();

  /* FIXME: This seems wrong.  MSDN says there are errors codes > 255. */
  why &= 0xff;

  for (i = 0; errmap[i].w != 0; ++i)
    if (why == errmap[i].w)
      break;

  if (errmap[i].w != 0)
    {
      if (strace() &((_STRACE_SYSCALL)|1))
	strace_printf ("%s:%d seterrno: %d (%s) -> %d\n", 
		      file, line,why, errmap[i].s, errmap[i].e);
      set_errno (errmap[i].e);
    }
  else
    {
      if (strace() &((_STRACE_SYSCALL)|1)) \
	strace_printf ("%s:%d seterrno: unknown error %d!!\n", file, line, why);
      set_errno (EACCES);
    }
}

extern char *_user_strerror _PARAMS ((int));

/* CYGWIN32 internal */
/* strerror: convert from errno values to error strings */
extern "C" char *
strerror (int errnum)
{
  char *error;
  switch (errnum)
    {
    case EPERM:
      error = "Not owner";
      break;
    case ENOENT:
      error = "No such file or directory";
      break;
    case ESRCH:
      error = "No such process";
      break;
    case EINTR:
      error = "Interrupted system call";
      break;
    case EIO:
      error = "I/O error";
      break;
    case ENXIO:
      error = "No such device or address";
      break;
    case E2BIG:
      error = "Arg list too long";
      break;
    case ENOEXEC:
      error = "Exec format error";
      break;
    case EBADF:
      error = "Bad file number";
      break;
    case ECHILD:
      error = "No children";
      break;
    case EAGAIN:
      error = "No more processes";
      break;
    case ENOMEM:
      error = "Not enough space";
      break;
    case EACCES:
      error = "Permission denied";
      break;
    case EFAULT:
      error = "Bad address";
      break;
    case ENOTBLK:
      error = "Block device required";
      break;
    case EBUSY:
      error = "Device or resource busy";
      break;
    case EEXIST:
      error = "File exists";
      break;
    case EXDEV:
      error = "Cross-device link";
      break;
    case ENODEV:
      error = "No such device";
      break;
    case ENOTDIR:
      error = "Not a directory";
      break;
    case EISDIR:
      error = "Is a directory";
      break;
    case EINVAL:
      error = "Invalid argument";
      break;
    case ENFILE:
      error = "Too many open files in system";
      break;
    case EMFILE:
      error = "Too many open files";
      break;
    case ENOTTY:
      error = "Not a character device";
      break;
    case ETXTBSY:
      error = "Text file busy";
      break;
    case EFBIG:
      error = "File too large";
      break;
    case ENOSPC:
      error = "No space left on device";
      break;
    case ESPIPE:
      error = "Illegal seek";
      break;
    case EROFS:
      error = "Read-only file system";
      break;
    case EMLINK:
      error = "Too many links";
      break;
    case EPIPE:
      error = "Broken pipe";
      break;
    case EDOM:
      error = "Math argument";
      break;
    case ERANGE:
      error = "Result too large";
      break;
    case ENOMSG:
      error = "No message of desired type";
      break;
    case EIDRM:
      error = "Identifier removed";
      break;
    case EDEADLK:
      error = "Deadlock";
      break;
    case ENOLCK:
      error = "No lock";
      break;
    case ENOSTR:
      error = "Not a stream";
      break;
    case ETIME:
      error = "Stream ioctl timeout";
      break;
    case ENOSR:
      error = "No stream resources";
      break;
    case ENONET:
      error = "Machine is not on the network";
      break;
    case ENOPKG:
      error = "No package";
      break;
    case EREMOTE:
      error = "Resource is remote";
      break;
    case ENOLINK:
      error = "Virtual circuit is gone";
      break;
    case EADV:
      error = "Advertise error";
      break;
    case ESRMNT:
      error = "Srmount error";
      break;
    case ECOMM:
      error = "Communication error";
      break;
    case EPROTO:
      error = "Protocol error";
      break;
    case EMULTIHOP:
      error = "Multihop attempted";
      break;
    case EBADMSG:
      error = "Bad message";
      break;
    case ELIBACC:
      error = "Cannot access a needed shared library";
      break;
    case ELIBBAD:
      error = "Accessing a corrupted shared library";
      break;
    case ELIBSCN:
      error = ".lib section in a.out corrupted";
      break;
    case ELIBMAX:
      error = "Attempting to link in more shared libraries than system limit";
      break;
    case ELIBEXEC:
      error = "Cannot exec a shared library directly";
      break;
    case ENOSYS:
      error = "Function not implemented";
      break;
    case ENMFILE:
      error = "No more files";
      break;
    case ENOTEMPTY:
      error = "Directory not empty";
      break;
    case ENAMETOOLONG:
      error = "File or path name too long";
      break;
    case ENOTSOCK:
      error = "The descriptor is a file, not a socket.";
      break;
    case ENOPROTOOPT:
      error ="This option is unsupported";
      break;
    case EAFNOSUPPORT:
      error = "Addresses in the specified family cannot be used with this socket.";
      break;
    case EISCONN:
      error = "The socket is already connected.";
      break;
    case ENETUNREACH:
      error ="The network can't be reached from this host at this time.";
      break;
    case ENOBUFS:
      error ="No buffer space is available.  The socket cannot be connected.";
      break;
    case ENOTCONN:
      error="The socket is not connected.";
      break;
    case ETIMEDOUT:
      error="Attempt to connect timed out without establishing a connection";
      break;
    case ENETDOWN:
      error = "Network failed.";
      break;
    case ECONNRESET:
      error ="The connection was reset by the remote side.";
      break;
    case ECONNABORTED:
      error = "The connection was aborted";
      break;
    case ELOOP:
      error = "Too many levels of symbolic links";
      break;
    default:
      static NO_COPY char buf[20];
      sprintf (buf, "error %d", errnum);
      error = buf;
      break;
    }

  return error;
}

