// regview.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRegView view

class CRegView : public CScrollView
{

protected:
  CRegView();           // protected constructor used by dynamic creation
  DECLARE_DYNCREATE(CRegView)

protected:	
  int m_format;

  CSize m_zero_size;
  int m_reg_name_width;
  int m_window_width;

  CPoint PrintReg (CDC*, int regnum, int format, CPoint);

// Attributes
public:
// Operations
public:
	static void Initialize();
	static void Terminate();




// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRegView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CRegView();

	// Generated message map functions
protected:
	//{{AFX_MSG(CRegView)
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnRegBinary();
	afx_msg void OnRegDecimal();
	afx_msg void OnRegEverything();
	afx_msg void OnRegHex();
	afx_msg void OnRegOctal();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFont();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

