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
// --> Include files <---------------------------------------------------------

#include "stdafx.h"
#include "mudFont.h"
#include "AddCharDlg.h"

// --> Static members <--------------------------------------------------------

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


BEGIN_MESSAGE_MAP(CAddCharDlg, CDialog)
	//{{AFX_MSG_MAP(CAddCharDlg)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// --> Class implementation <--------------------------------------------------


CAddCharDlg::CAddCharDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAddCharDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAddCharDlg)
	m_Chars = _T("");
	//}}AFX_DATA_INIT
}


void CAddCharDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddCharDlg)
	DDX_Control(pDX, IDC_EDIT2, m_String);
	DDX_Text(pDX, IDC_EDIT2, m_Chars);
	//}}AFX_DATA_MAP
}


void CAddCharDlg::OnDestroy() 
{
	CDialog::OnDestroy();
	UpdateData(TRUE);	
}


BOOL CAddCharDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_String.SetFocus();
	return FALSE;
}

// --> End of file <-----------------------------------------------------------
