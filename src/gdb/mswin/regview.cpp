// regview.cpp : implementation file
//

#include "stdafx.h"
#include "regdoc.h"
#include "regview.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CRegView

IMPLEMENT_DYNCREATE(CRegView, CScrollView)

extern CGuiApp theApp;

void redraw_allregwins()
{ 
  redraw_allwins(theApp.m_regTemplate);
}

CFontInfo reg_fontinfo  ("RegFont", redraw_allregwins);

CRegView::CRegView()
{
  m_format = 'x';
  reg_fontinfo.MakeFont();
}

CRegView::~CRegView()
{
}


BEGIN_MESSAGE_MAP(CRegView, CScrollView)
	//{{AFX_MSG_MAP(CRegView)
	ON_WM_CHAR()
	ON_COMMAND(ID_REAL_CMD_BUTTON_REG_BINARY, OnRegBinary)
	ON_COMMAND(ID_REAL_CMD_BUTTON_REG_DECIMAL, OnRegDecimal)
	ON_COMMAND(ID_REAL_CMD_BUTTON_REG_EVERYTHING, OnRegEverything)
	ON_COMMAND(ID_REAL_CMD_BUTTON_REG_HEX, OnRegHex)
	ON_COMMAND(ID_REAL_CMD_BUTTON_REG_OCTAL, OnRegOctal)
	ON_WM_SIZE()
	ON_COMMAND(ID_REAL_CMD_BUTTON_SET_FONT, OnSetFont)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CRegView drawing

static int
guess_size (int bits, int format)
// Guess how many characters a printed value can be.
{
  switch (format)
    {
      default:
      case 'x':
        return 2 + (bits + 3)/4;
      case 'o':
        return 1 + (bits + 2)/3;
      case 't':
        return bits;
      case 'd':
        return 1 + (bits + 2)/3;    // actually, 3.322
      case 'f':
        return 3 + (bits + 2)/3;    // another approximation
    }
}

CPoint
CRegView::PrintReg (CDC* pDC, int regnum, int format, CPoint point)
// Returns the coordinates of the end of text displayed.
{
  CRegDoc* pDoc = (CRegDoc*) GetDocument ();

  int bits = pDoc->register_bits (regnum);
  int chars = guess_size (bits, format);

  char * all_formats = "dx";
  if (format == '*')
    {
      chars = -1;
      for (char* f = all_formats; *f; ++f)
        chars += guess_size (bits, *f) + 1;
    }
      
  // if we overflow the right margin, start a new line.
      
  if (m_window_width < point.x + m_reg_name_width + (1 + chars) * m_zero_size.cx)
    {
      point.x = 0;
      point.y += m_zero_size.cy;
    }
  
  // print register name.

  pDC->SetTextColor (0x00000000);
  pDC->TextOut (point.x, point.y, reg_names[regnum]);
  point.x += m_reg_name_width;

  point.x += m_zero_size.cx;

  // print value
  
  if (pDoc->register_changed_p (regnum))
    pDC->SetTextColor (0x000000ff);
  else  
    pDC->SetTextColor (0x00000000);
      
  if (format == '*')
    {
      for (char* f = all_formats; *f; ++f)
        {
          chars = guess_size (bits, *f);
          CString value = pDoc->get_register (regnum, *f);
          CSize value_size = pDC->GetTextExtent (value, value.GetLength ());
          int x = point.x + m_zero_size.cx * chars - value_size.cx;
          pDC->TextOut (x, point.y, value);
          point.x += m_zero_size.cx * (chars + 1);
	}
      point.x -= m_zero_size.cx;
    }
  else
    {
      CString value = pDoc->get_register (regnum, format);
      CSize value_size = pDC->GetTextExtent (value, value.GetLength ());

      int x = point.x;
      // right-align integer values.
      if (format != 'f')
        x += m_zero_size.cx * chars - value_size.cx;
      pDC->TextOut (x, point.y, value);
      point.x += m_zero_size.cx * chars;
    }
  
  return point;
}


// FIXME: this routine does way too much computation, since most
// of the layout info doesn't change that often.

void
CRegView::OnDraw (CDC* pDC)
{
  int regnum;
  
  CRegDoc* pDoc = (CRegDoc *) GetDocument ();
  
  pDoc->prepare ();
  pDC->SelectObject (&reg_fontinfo.m_font);
  
  // Calculate some sizes

  m_zero_size = pDC->GetTextExtent ("0", 1);
  m_reg_name_width = 0;
  for (regnum = 0; regnum < NUM_REGS; ++regnum)
    if (pDoc->isa_register (regnum))
      {
        char * name = reg_names[regnum];
        int w = (pDC->GetTextExtent (name, strlen (name))).cx;
        m_reg_name_width = (m_reg_name_width < w) ? w : m_reg_name_width;
      }

  RECT window_rect;
  GetClientRect (&window_rect);
  m_window_width = window_rect.right;

  // Start drawing..
  
  CPoint here (0, 0);
  CSize maxsize (0, 0);
  
  // Current instruction
  
  char buf[200];  // FIXME: should probably use CString
  togdb_disassemble (togdb_stop_pc (), buf);
  int len = strlen(buf);
  pDC->TabbedTextOut (here.x, here.y, buf, len, 0, 0, here.x);
  here += pDC->GetTabbedTextExtent (buf, len, 0, 0);
  
  if (maxsize.cx < here.x)
    maxsize.cx = here.x;
  
  here.x = 0;
  here.y += m_zero_size.cy / 2;

  // Pass 1.  integer registers
  
  for (regnum = 0; regnum < NUM_REGS; ++regnum)
    {
      if ( !pDoc->isa_register (regnum) || pDoc->isa_float_register (regnum) )
        continue;
      
      here = PrintReg (pDC, regnum, m_format, here);
      if (maxsize.cx < here.x)
        maxsize.cx = here.x;
      here.x += 2 * m_zero_size.cx;
    }
  
  here.x = 0;
  here.y += m_zero_size.cy + m_zero_size.cy/2;
  
  // Pass 2.  float registers
  
  for (regnum = 0; regnum < NUM_REGS; ++regnum)
    {
      if ( !pDoc->isa_register (regnum) || !pDoc->isa_float_register (regnum) )
        continue;
  
      here = PrintReg(pDC, regnum, 'f', here);
      if (maxsize.cx < here.x)
        maxsize.cx = here.x;
      here.x += 2 * m_zero_size.cx;
    }
  
  maxsize.cy = here.y + m_zero_size.cy;
  CSize page_size (window_rect.right, window_rect.bottom);
  SetScrollSizes (MM_TEXT, maxsize, page_size, m_zero_size);
}


/////////////////////////////////////////////////////////////////////////////
// CRegView message handlers
void CRegView::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
}

void CRegView::OnRegBinary() 
{
  m_format = 't';
  Invalidate();	
}

void CRegView::OnRegDecimal() 
{
  m_format = 'd';
  Invalidate();		
}

void CRegView::OnRegEverything() 
{
  m_format = '*';
  Invalidate();	
}

void CRegView::OnRegHex() 
{
  m_format = 'x';
  Invalidate();	
}

void CRegView::OnRegOctal() 
{
  m_format = 'o';
  Invalidate();	
}


void CRegView::Initialize()
{
  reg_fontinfo.Initialize();
}

void CRegView::Terminate()
{
  reg_fontinfo.Terminate();
}

void CRegView::OnSize(UINT nType, int cx, int cy) 
{
  CScrollView::OnSize(nType, cx, cy);
  SetWindowText("Registers");
}

void CRegView::OnSetFont()
{
  reg_fontinfo.OnChooseFont();
  Invalidate();
}

BOOL CRegView::PreCreateWindow(CREATESTRUCT& cs) 
{
  return CScrollView::PreCreateWindow(cs);
}

void CRegView::OnInitialUpdate() 
{
  CScrollView::OnInitialUpdate();
#if 0
  GetParentFrame()->SetWindowPos(NULL, 0, 0, 
			WIDTHINPIX, DEPTHINPIX,
				 SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
#endif  
  load_where(GetParentFrame(),"REG");
}

void CRegView::OnDestroy() 
{
  CScrollView::OnDestroy();
  save_where(GetParentFrame(),"REG");
}

