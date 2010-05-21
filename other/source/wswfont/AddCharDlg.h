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
#if !defined(AFX_ADDCHARDLG_H__4FD6C119_A4B4_4EA2_9596_07297335EDD2__INCLUDED_)
#define AFX_ADDCHARDLG_H__4FD6C119_A4B4_4EA2_9596_07297335EDD2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AddCharDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAddCharDlg dialog

class CAddCharDlg : public CDialog
{
// Construction
public:
	CAddCharDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAddCharDlg)
	enum { IDD = IDD_ADDCHAR };
	CEdit	m_String;
	CString	m_Chars;
	//}}AFX_DATA

	const CString& GetString() const { return m_Chars; }


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddCharDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAddCharDlg)
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDCHARDLG_H__4FD6C119_A4B4_4EA2_9596_07297335EDD2__INCLUDED_)
