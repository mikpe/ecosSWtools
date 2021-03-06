/*
 * tkWinInt.h --
 *
 *	This file contains declarations that are shared among the
 *	Windows-specific parts of Tk, but aren't used by the rest of
 *	Tk.
 *
 * Copyright (c) 1995 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * SCCS: @(#) tkWinInt.h 1.34 97/09/02 13:06:20
 */

#ifndef _TKWININT
#define _TKWININT

#ifndef _TKINT
#include "tkInt.h"
#endif

/*
 * Include platform specific public interfaces.
 */

#ifndef _TKWIN
#include "tkWin.h"
#endif

/*
 * Define constants missing from older Win32 SDK header files.
 */

#ifndef WS_EX_TOOLWINDOW
#define WS_EX_TOOLWINDOW	0x00000080L 
#endif

#ifndef __GNUC__
/* gcc won't let us do this--it causes a conflict with the typedef in
   tkFont.h (as it should).  */
typedef struct TkFontAttributes TkFontAttributes;
#endif

/*
 * The TkWinDCState is used to save the state of a device context
 * so that it can be restored later.
 */

typedef struct TkWinDCState {
    HPALETTE palette;
} TkWinDCState;

/*
 * The TkWinDrawable is the internal implementation of an X Drawable (either
 * a Window or a Pixmap).  The following constants define the valid Drawable
 * types.
 */

#define TWD_BITMAP	1
#define TWD_WINDOW	2
#define TWD_WINDC	3

typedef struct {
    int type;
    HWND handle;
    TkWindow *winPtr;
} TkWinWindow;

typedef struct {
    int type;
    HBITMAP handle;
    Colormap colormap;
    int depth;
} TkWinBitmap;

typedef struct {
    int type;
    HDC hdc;
}TkWinDC;

typedef union {
    int type;
    TkWinWindow window;
    TkWinBitmap bitmap;
    TkWinDC winDC;
} TkWinDrawable;

/*
 * The following macros are used to retrieve internal values from a Drawable.
 */

#define TkWinGetHWND(w) (((TkWinDrawable *) w)->window.handle)
#define TkWinGetWinPtr(w) (((TkWinDrawable*)w)->window.winPtr)
#define TkWinGetHBITMAP(w) (((TkWinDrawable*)w)->bitmap.handle)
#define TkWinGetColormap(w) (((TkWinDrawable*)w)->bitmap.colormap)
#define TkWinGetHDC(w) (((TkWinDrawable *) w)->winDC.hdc)

/*
 * The following structure is used to encapsulate palette information.
 */

typedef struct {
    HPALETTE palette;		/* Palette handle used when drawing. */
    UINT size;			/* Number of entries in the palette. */
    int stale;			/* 1 if palette needs to be realized,
				 * otherwise 0.  If the palette is stale,
				 * then an idle handler is scheduled to
				 * realize the palette. */
    Tcl_HashTable refCounts;	/* Hash table of palette entry reference counts
				 * indexed by pixel value. */
} TkWinColormap;

/*
 * The following macro retrieves the Win32 palette from a colormap.
 */

#define TkWinGetPalette(colormap) (((TkWinColormap *) colormap)->palette)

/*
 * The following macros define the class names for Tk Window types.
 */

#define TK_WIN_TOPLEVEL_CLASS_NAME "TkTopLevel"
#define TK_WIN_CHILD_CLASS_NAME "TkChild"

/*
 * The following variable indicates whether we are restricted to Win32s
 * GDI calls.
 */

extern int tkpIsWin32s;

/*
 * The following variable is a translation table between X gc functions and
 * Win32 raster op modes.
 */

extern int tkpWinRopModes[];

/*
 * The following defines are used with TkWinGetBorderPixels to get the
 * extra 2 border colors from a Tk_3DBorder.
 */

#define TK_3D_LIGHT2 TK_3D_DARK_GC+1
#define TK_3D_DARK2 TK_3D_DARK_GC+2

/*
 * Internal procedures used by more than one source file.
 */

extern LRESULT CALLBACK	TkWinChildProc _ANSI_ARGS_((HWND hwnd, UINT message,
			    WPARAM wParam, LPARAM lParam));
extern void		TkWinClipboardRender _ANSI_ARGS_((TkDisplay *dispPtr,
			    UINT format));
extern LRESULT		TkWinEmbeddedEventProc _ANSI_ARGS_((HWND hwnd,
			    UINT message, WPARAM wParam, LPARAM lParam));
extern void		TkWinFillRect _ANSI_ARGS_((HDC dc, int x, int y,
			    int width, int height, int pixel));
extern COLORREF		TkWinGetBorderPixels _ANSI_ARGS_((Tk_Window tkwin,
			    Tk_3DBorder border, int which));
extern HDC		TkWinGetDrawableDC _ANSI_ARGS_((Display *display,
			    Drawable d, TkWinDCState* state));
extern int		TkWinGetModifierState _ANSI_ARGS_((void));
extern HPALETTE		TkWinGetSystemPalette _ANSI_ARGS_((void));
extern HWND		TkWinGetWrapperWindow _ANSI_ARGS_((Tk_Window tkwin));
extern int		TkWinHandleMenuEvent _ANSI_ARGS_((HWND *phwnd,
			    UINT *pMessage, WPARAM *pwParam, LPARAM *plParam,
			    LRESULT *plResult));
extern int		TkWinIndexOfColor _ANSI_ARGS_((XColor *colorPtr));
extern void		TkWinPointerDeadWindow _ANSI_ARGS_((TkWindow *winPtr));
extern void		TkWinPointerEvent _ANSI_ARGS_((HWND hwnd, int x,
			    int y));
extern void		TkWinPointerInit _ANSI_ARGS_((void));
extern LRESULT 		TkWinReflectMessage _ANSI_ARGS_((HWND hwnd,
			    UINT message, WPARAM wParam, LPARAM lParam));
extern void		TkWinReleaseDrawableDC _ANSI_ARGS_((Drawable d,
			    HDC hdc, TkWinDCState* state));
extern LRESULT		TkWinResendEvent _ANSI_ARGS_((WNDPROC wndproc,
			    HWND hwnd, XEvent *eventPtr));
extern HPALETTE		TkWinSelectPalette _ANSI_ARGS_((HDC dc,
			    Colormap colormap));
extern void		TkWinSetMenu _ANSI_ARGS_((Tk_Window tkwin,
			    HMENU hMenu));
extern void		TkWinSetWindowPos _ANSI_ARGS_((HWND hwnd,
			    HWND siblingHwnd, int pos));
extern void		TkWinUpdateCursor _ANSI_ARGS_((TkWindow *winPtr));
extern void		TkWinWmCleanup _ANSI_ARGS_((HINSTANCE hInstance));
extern HWND		TkWinWmFindEmbedAssociation _ANSI_ARGS_((
			    TkWindow *winPtr));
extern void		TkWinWmStoreEmbedAssociation _ANSI_ARGS_((
			    TkWindow *winPtr, HWND hwnd));
extern void		TkWinXCleanup _ANSI_ARGS_((HINSTANCE hInstance));
extern void 		TkWinXInit _ANSI_ARGS_((HINSTANCE hInstance));

/* CYGNUS LOCAL.  */
extern void		TkWinNCMetricsChanged _ANSI_ARGS_((Tk_Window tkwin));
extern void		TkWinSysColorChange _ANSI_ARGS_((void));

#endif /* _TKWININT */

