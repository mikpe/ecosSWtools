// regdoc.cpp : implementation file
//

#include "stdafx.h"
#include "regdoc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRegDoc

IMPLEMENT_DYNCREATE(CRegDoc, CDocument)


CRegDoc::CRegDoc()
{
  memset(old_regs, sizeof(old_regs), 0);
  memset(reg_changed, sizeof(reg_changed), 0);
  m_init = 0;
}

BOOL CRegDoc::OnNewDocument()
{
  if (!CDocument::OnNewDocument())
    return FALSE;

  return TRUE;
}

CRegDoc::~CRegDoc()
{
  // nothing
}


BEGIN_MESSAGE_MAP(CRegDoc, CDocument)
	//{{AFX_MSG_MAP(CRegDoc)
	ON_COMMAND(ID_REAL_CMD_BUTTON_SYNC, OnSync)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()



/////////////////////////////////////////////////////////////////////////////
// CRegDoc serialization

void CRegDoc::Serialize(CArchive& ar)
{
  // nothing
}

/////////////////////////////////////////////////////////////////////////////
// CRegDoc commands


void CRegDoc::OnSync() 
{
  // TODO: Add your command handler code here
  Sync();	
}


void CRegDoc::OnCloseDocument() 
{
	// TODO: Add your specialized code here and/or call the base class
	
	m_bModified = FALSE; 
	CDocument::OnCloseDocument();
}


void CRegDoc::prepare()
{
  if (!m_init)
    {
      Sync();
      m_init = 1;
    }
}

BOOL CRegDoc::SaveModified() 
{
  // TODO: Add your specialized code here and/or call the base class
  return TRUE; // CDocument::SaveModified();
}


int
CRegDoc::register_changed_p (int regnum)
{
  return reg_changed[regnum];
}

int
CRegDoc::isa_register (int regnum)
{
  return 
    (0 <= regnum) &&
    (regnum < NUM_REGS) &&
    (reg_names[regnum] != NULL) &&
    (*reg_names[regnum] != '\0');
}

int
CRegDoc::register_bits (int regnum)
{
  return HOST_CHAR_BIT * TYPE_LENGTH (REGISTER_VIRTUAL_TYPE (regnum));
}

int
CRegDoc::isa_float_register (int regnum)
{
  return TYPE_CODE (REGISTER_VIRTUAL_TYPE (regnum)) == TYPE_CODE_FLT;
}

CString
CRegDoc::get_register (int regnum, int format)
{
  CString result;
  Credirect dummy(&result);

  value_ptr reg_val = value_of_register(regnum);

  print_scalar_formatted
    (VALUE_CONTENTS (reg_val), REGISTER_VIRTUAL_TYPE (regnum),
     format, 0, gdb_stdout);

  return result;
}


void CRegDoc::Sync()
{
  int regnum;
  char raw_buffer[MAX_REGISTER_RAW_SIZE];
  int any_changes = 0;
  int changed_now[NUM_REGS];

  // Scan for changes
  for (regnum = 0; regnum < NUM_REGS; ++regnum)
    {
      changed_now[regnum] = 0;
      if (isa_register (regnum))
	{
          read_relative_register_raw_bytes (regnum, raw_buffer);        

	  int byte = REGISTER_BYTE (regnum);
	  int size = REGISTER_RAW_SIZE (regnum);
          if (memcmp (&old_regs[byte], raw_buffer, size) != 0)
            {
              memcpy (&old_regs[byte], raw_buffer, size);
              changed_now[regnum] = 1;
              any_changes = 1;
            }
	}
    }

  // Update our idea of what changed.
  if (m_init && any_changes)
    memcpy (reg_changed, changed_now, sizeof reg_changed);

  if (any_changes)
    UpdateAllViews (0);
}


