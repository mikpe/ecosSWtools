#ifndef APIENTRY
#define APIENTRY
#endif
#define W32SUT_16

#include "windows.h"
#include <w32sut.h>
#include "serdll.h"


int FAR PASCAL
LibMain (HANDLE hLibInst, WORD wDataSeg,
	 WORD cbHeapSize, LPSTR lpszCmdLine)
{
  return (1);
}				// LibMain()

DWORD FAR PASCAL __export
UTInit (UT16CBPROC lpfnUT16CallBack, LPVOID lpBuf)
{
  return (1);			// Return Success

}				// UTInit()


DWORD FAR PASCAL __export
UTProc (DWORD * lpArgs, DWORD dwFunc)
{
  switch (dwFunc)
    {
    case BUILDCOMMDCB:
      return BuildCommDCB ((LPCSTR) lpArgs[0],
			   (LPVOID) lpArgs[1]);

    case OPENCOMM:
      {

	int i;
	DCB dcb;
	char *dev = (LPSTR) lpArgs[0];
	char *comm_string = (LPSTR) lpArgs[3];
	int j;
	memset (&dcb, 0, sizeof (dcb));

	i = OpenComm (dev, 512, 512);

	if (i < 0)
	  return i;
#if 1
	BuildCommDCB (comm_string, &dcb);
	dcb.Id = i;
	dcb.fBinary = 1;
	dcb.fOutxCtsFlow = 0;
	dcb.fOutxDsrFlow = 0;
	dcb.fOutX = 0;
	dcb.fInX = 0;
	j = SetCommState (&dcb);
	if (j)
	  OutputDebugString ("SetCmmstate failed\n");

#endif
	return i;
      }


    case CLOSECOMM:
      return (CloseComm ((int) lpArgs[0]));


    case READCOMM:
      return ReadComm ((int) lpArgs[0],
		       (LPSTR) lpArgs[1],
		       (int) lpArgs[2]);

    case WRITECOMM:
      return (WriteComm ((int) lpArgs[0],
			 (LPSTR) lpArgs[1],
			 (int) lpArgs[2]));



    case FLUSHCOMM:
      return FlushComm ((int) lpArgs[0], (int) lpArgs[1]);

    case TRANSMITCOMMCHAR:
      return UngetCommChar ((int) lpArgs[0], (char) lpArgs[1]);

    case SETCOMMSTATE:
      return SetCommState ((LPVOID) lpArgs[0]);


    case GETCOMMSTATE:
      return (GetCommState ((int) lpArgs[0],
			    (LPVOID) lpArgs[1]));


    case GETCOMMERROR:
      {
	int x;
	COMSTAT *foo = (LPVOID *) lpArgs[1];
	COMSTAT local;
	x = GetCommError ((int) lpArgs[0], &local);
	return x;
      }

    case GETCOMMREADY:
      {
	int x;
	COMSTAT local;
	int cid = lpArgs[0];

	x = GetCommError (cid, &local);

	if (local.cbInQue)
	  {
	    char *buf = (char *) lpArgs[1];
	    char c;
	    int len = ReadComm (cid, buf, local.cbInQue);
	    if (len < 0)
	      {
		int err;
		len = -len;
		err = GetCommError (cid, NULL);
		GetCommEventMask (cid, 0xffff);
	      }
	    buf[len] = 0;
	    return len;
	  }
	return 0;
      }


    }

  return ((DWORD) - 1L);
}
