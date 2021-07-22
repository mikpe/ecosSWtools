// regdoc.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRegDoc document

class CRegDoc : public CDocument
{
public:
  void Sync();
  void prepare();

  int isa_register (int regnum);
  int isa_float_register (int regnum);

  int register_bits (int regnum);

  CString get_register (int regnum, int format);
  int register_changed_p (int regnum);
  
protected:
  CRegDoc();			// protected constructor used by dynamic creation
  DECLARE_DYNCREATE(CRegDoc)

private:
  int m_init;
  char old_regs[REGISTER_BYTES];
  int reg_changed[NUM_REGS];

  // ClassWizard generated virtual function overrides
  //{{AFX_VIRTUAL(CRegDoc)
public:
  virtual void OnCloseDocument();
protected:
  virtual BOOL OnNewDocument();
  virtual BOOL SaveModified();
  //}}AFX_VIRTUAL
  
  // Implementation
public:
  virtual ~CRegDoc();
  virtual void Serialize(CArchive& ar);	// overridden for document i/o
  
  // Generated message map functions
protected:
  //{{AFX_MSG(CRegDoc)
  afx_msg void OnSync();
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()
  };
