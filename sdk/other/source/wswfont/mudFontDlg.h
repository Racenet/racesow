/**
 * Copyright (c) 2003 mudGE Entertainment
 * 
 * This software is provided 'as-is', without any express or implied warranty. 
 * In no event will the authors be held liable for any damages arising 
 * from the use of this software.
 * 
 * Permission is granted to anyone to use this software for any purpose, 
 * including commercial applications, and to alter it and redistribute 
 * it freely, subject to the following restrictions:
 * 
 * 1. The origin of this software must not be misrepresented; 
 *    you must not claim that you wrote the original software. 
 *    If you use this software in a product, an acknowledgment 
 *    in the product documentation would be appreciated but is not required.
 * 
 * 2. Altered source versions must be plainly marked as such, and must 
 *    not be misrepresented as being the original software.
 * 
 * 3. This notice may not be removed or altered from any source distribution.
 * 
 */
#if !defined(AFX_MUDFONTDLG_H__CFFC602D_8F6E_4340_B3EC_ADCD55315D0F__INCLUDED_)
#define AFX_MUDFONTDLG_H__CFFC602D_8F6E_4340_B3EC_ADCD55315D0F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CMudFontDlg dialog

struct cCharacter
{
	_TCHAR	m_Char;
	int		m_bLeft;
	int		m_bRight;

	CPoint	m_Pos;	// generated.
	CSize	m_Size;	// generated.
};

typedef CArray<cCharacter, const cCharacter&> CCharArray;



class CMudFontDlg : public CDialog
{
// Construction
public:
	CMudFontDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CMudFontDlg)
	enum { IDD = IDD_MUDFONT };
	CComboBox	m_AA;
	CSpinButtonCtrl	m_cbrSpin;
	CEdit			m_cbrEdit;
	CSpinButtonCtrl	m_cblSpin;
	CEdit			m_cblEdit;
	CComboBox		m_CharSel;
	CSpinButtonCtrl	m_btSpin;
	CEdit			m_btEdit;
	CSpinButtonCtrl	m_brSpin;
	CEdit			m_brEdit;
	CSpinButtonCtrl	m_blSpin;
	CEdit			m_blEdit;
	CSpinButtonCtrl	m_bbSpin;
	CEdit			m_bbEdit;
	CComboBox		m_Weight;
	CSpinButtonCtrl	m_SizeSpin;
	CEdit			m_SizeEdit;
	CComboBox		m_Quality;
	CComboBox		m_FontName;
	CSliderCtrl		m_Zoom;
	CScrollBar		m_Up;
	CScrollBar		m_Right;
	CStatic			m_View;
	CString			m_ZoomText;
	int				m_Height;
	int				m_Width;
	BOOL			m_Italic;
	BOOL			m_ShowRect;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMudFontDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CMudDib				m_Dib;
	int					m_TextureWidth;
	int					m_TextureHeight;
	CCharArray			m_Chars;
	CDC					m_MemDC;
	CMudDib				m_MemDib;
	CFont				m_DrawFont;
	bool				m_Running;
	CRect				m_Border;

	void		SetupFont();
	void		UpdateTextureSize();
	void		UpdateTexture();
	void		UpdateSizes();
	void		UpdatePositions();
	void		DrawChars();
	void		UpdateUI(int nHeight, int nWeight, int nQuality, int nSampling, const CString& Facename);

	void		InvalidateView();
	void		AddCharacter(_TCHAR a_Char);
	void		ExportTGA(const _TCHAR* a_Filename);
	void		ExportXML(const _TCHAR* a_Filename);
	void		ExportWSW(const _TCHAR* a_Filename); // wsw : jal : warsow font
	
	static int CALLBACK EnumFontsProc(CONST LOGFONT* lplf, CONST TEXTMETRIC* lptm, DWORD dwType, LPARAM lpData);

	//{{AFX_MSG(CMudFontDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnSizeChange();
	afx_msg void OnFontChange();
	afx_msg void OnRectChange();
	afx_msg void OnCharChange();
	afx_msg void OnSelchangeChar();
	afx_msg void OnToolsAddchar();
	afx_msg void OnToolsAddWesternCharset();
	afx_msg void OnToolsAddLatinA();
	afx_msg void OnToolsAddCyrillic();
	afx_msg void OnFileExit();
	afx_msg void OnFileExport();
	afx_msg void OnFileNew();
	afx_msg void OnFileOpen();
	afx_msg void OnFileSave();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MUDFONTDLG_H__CFFC602D_8F6E_4340_B3EC_ADCD55315D0F__INCLUDED_)
