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
#include "addchardlg.h"
#include "mudFontDlg.h"

// --> Static members <--------------------------------------------------------

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


BEGIN_MESSAGE_MAP(CMudFontDlg, CDialog)
	//{{AFX_MSG_MAP(CMudFontDlg)
	ON_WM_PAINT()
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_H64, OnSizeChange)
	ON_BN_CLICKED(IDC_ITALIC, OnFontChange)
	ON_BN_CLICKED(IDC_RECT, OnRectChange)
	ON_EN_CHANGE(IDC_CBL_EDIT, OnCharChange)
	ON_CBN_SELCHANGE(IDC_CHAR, OnSelchangeChar)
	ON_COMMAND(ID_TOOLS_ADDCHAR, OnToolsAddchar)
	ON_COMMAND(ID_TOOLS_ADDWESTERN, OnToolsAddWesternCharset)
	ON_COMMAND(ID_TOOLS_ADDLATIN_A, OnToolsAddLatinA)
	ON_COMMAND(ID_TOOLS_ADDCYRILLIC, OnToolsAddCyrillic)
	ON_COMMAND(ID_FILE_EXIT, OnFileExit)
	ON_COMMAND(ID_FILE_EXPORT, OnFileExport)
	ON_COMMAND(ID_FILE_NEW, OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
	ON_BN_CLICKED(IDC_H512, OnSizeChange)
	ON_BN_CLICKED(IDC_H256, OnSizeChange)
	ON_BN_CLICKED(IDC_H128, OnSizeChange)
	ON_BN_CLICKED(IDC_W64, OnSizeChange)
	ON_BN_CLICKED(IDC_W512, OnSizeChange)
	ON_BN_CLICKED(IDC_W256, OnSizeChange)
	ON_BN_CLICKED(IDC_W128, OnSizeChange)
	ON_CBN_SELCHANGE(IDC_QUALITY, OnFontChange)
	ON_EN_CHANGE(IDC_SIZE_EDIT, OnFontChange)
	ON_CBN_SELCHANGE(IDC_WEIGHT, OnFontChange)
	ON_CBN_SELCHANGE(IDC_FONT, OnFontChange)
	ON_EN_CHANGE(IDC_BB_EDIT, OnFontChange)
	ON_EN_CHANGE(IDC_BL_EDIT, OnFontChange)
	ON_EN_CHANGE(IDC_BR_EDIT, OnFontChange)
	ON_EN_CHANGE(IDC_BT_EDIT, OnFontChange)
	ON_EN_CHANGE(IDC_CBR_EDIT, OnCharChange)
	ON_CBN_SELCHANGE(IDC_AA, OnFontChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


// --> Class implementation <--------------------------------------------------

CMudFontDlg::CMudFontDlg(CWnd* pParent /*=NULL*/): 
	CDialog(CMudFontDlg::IDD, pParent),
	m_Running(false)
{
	//{{AFX_DATA_INIT(CMudFontDlg)
	m_ZoomText = _T("");
	m_Height = -1;
	m_Width = -1;
	m_Italic = FALSE;
	m_ShowRect = FALSE;
	//}}AFX_DATA_INIT
}


void CMudFontDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMudFontDlg)
	DDX_Control(pDX, IDC_AA, m_AA);
	DDX_Control(pDX, IDC_CBR_SPIN, m_cbrSpin);
	DDX_Control(pDX, IDC_CBR_EDIT, m_cbrEdit);
	DDX_Control(pDX, IDC_CBL_SPIN, m_cblSpin);
	DDX_Control(pDX, IDC_CBL_EDIT, m_cblEdit);
	DDX_Control(pDX, IDC_CHAR, m_CharSel);
	DDX_Control(pDX, IDC_BT_SPIN, m_btSpin);
	DDX_Control(pDX, IDC_BT_EDIT, m_btEdit);
	DDX_Control(pDX, IDC_BR_SPIN, m_brSpin);
	DDX_Control(pDX, IDC_BR_EDIT, m_brEdit);
	DDX_Control(pDX, IDC_BL_SPIN, m_blSpin);
	DDX_Control(pDX, IDC_BL_EDIT, m_blEdit);
	DDX_Control(pDX, IDC_BB_SPIN, m_bbSpin);
	DDX_Control(pDX, IDC_BB_EDIT, m_bbEdit);
	DDX_Control(pDX, IDC_WEIGHT, m_Weight);
	DDX_Control(pDX, IDC_SIZE_SPIN, m_SizeSpin);
	DDX_Control(pDX, IDC_SIZE_EDIT, m_SizeEdit);
	DDX_Control(pDX, IDC_QUALITY, m_Quality);
	DDX_Control(pDX, IDC_FONT, m_FontName);
	DDX_Control(pDX, IDC_ZOOM, m_Zoom);
	DDX_Control(pDX, IDC_UP, m_Up);
	DDX_Control(pDX, IDC_RIGHT, m_Right);
	DDX_Control(pDX, IDC_VIEW, m_View);
	DDX_Text(pDX, IDC_ZOOM_TEXT, m_ZoomText);
	DDX_Radio(pDX, IDC_H64, m_Height);
	DDX_Radio(pDX, IDC_W64, m_Width);
	DDX_Check(pDX, IDC_ITALIC, m_Italic);
	DDX_Check(pDX, IDC_RECT, m_ShowRect);
	//}}AFX_DATA_MAP
}


BOOL CMudFontDlg::OnInitDialog()
{
	int index;
	CDialog::OnInitDialog();

	// set initial values.
	m_Zoom.SetRange(100,1000, TRUE);
	m_Zoom.SetPos(100);
	m_Zoom.SetTicFreq(50);
	m_View.ShowWindow(SW_HIDE);
	m_ZoomText.Format(_T("%d%%"), m_Zoom.GetPos());
	m_Width = 2;
	m_Height = 2;
	m_Border = CRect(0,0,0,0);
	UpdateData(FALSE);

	// enumerate fonts.
	CDC* pDC = GetDC();
	EnumFonts(pDC->GetSafeHdc(), NULL, (FONTENUMPROC)EnumFontsProc, (LPARAM)&m_FontName);
	m_FontName.SetCurSel(0);

	// add anti-aliasing.
	index = m_AA.AddString(_T("(none)"));
	m_AA.SetItemData(index, 1);
	index = m_AA.AddString(_T("2x2"));
	m_AA.SetItemData(index, 2);
	index = m_AA.AddString(_T("3x3"));
	m_AA.SetItemData(index, 3);
	index = m_AA.AddString(_T("4x4"));
	m_AA.SetItemData(index, 4);
	m_AA.SetCurSel(0);

	// add qualities.
	index = m_Quality.AddString(_T("Antialiased"));
	m_Quality.SetItemData(index, ANTIALIASED_QUALITY);
	index = m_Quality.AddString(_T("non-Antialiased"));
	m_Quality.SetItemData(index, NONANTIALIASED_QUALITY);
	m_Quality.SetCurSel(0);

#ifdef CLEARTYPE_QUALITY
	index = m_Quality.AddString(_T("Cleartype"));
	m_Quality.SetItemData(index, CLEARTYPE_QUALITY);
#endif

	// add weights.
	index = m_Weight.AddString(_T("Normal"));
	m_Weight.SetItemData(index, FW_NORMAL);
	index = m_Weight.AddString(_T("Medium"));
	m_Weight.SetItemData(index, FW_SEMIBOLD);
	index = m_Weight.AddString(_T("Bold"));
	m_Weight.SetItemData(index, FW_BOLD);
	m_Weight.SetCurSel(0);

	// setup size control.
	m_SizeSpin.SetBuddy(&m_SizeEdit);
	m_SizeSpin.SetRange(5, 100);
	m_SizeSpin.SetPos(11);

	// setup border controls.
	m_blSpin.SetBuddy(&m_blEdit);
	m_blSpin.SetRange(0, 16);
	m_blSpin.SetPos(0);
	m_brSpin.SetBuddy(&m_brEdit);
	m_brSpin.SetRange(0, 16);
	m_brSpin.SetPos(0);
	m_bbSpin.SetBuddy(&m_bbEdit);
	m_bbSpin.SetRange(0, 16);
	m_bbSpin.SetPos(0);
	m_btSpin.SetBuddy(&m_btEdit);
	m_btSpin.SetRange(0, 16);
	m_btSpin.SetPos(0);

	m_cblSpin.SetBuddy(&m_cblEdit);
	m_cblSpin.SetRange(0, 16);
	m_cblSpin.SetPos(0);
	m_cbrSpin.SetBuddy(&m_cbrEdit);
	m_cbrSpin.SetRange(0, 16);
	m_cbrSpin.SetPos(0);

	// setup initial font.
	SetupFont();

	// create the dibs for the font.
	CPaintDC dc(this);
	m_MemDC.CreateCompatibleDC(&dc);
	m_MemDib.Create(512, 512, m_MemDC.GetSafeHdc());
	m_Dib.Create(512, 512);
	
	// force one event.
	OnSizeChange();
	m_Running = true;
	return TRUE;
}


void CMudFontDlg::OnPaint() 
{
	CPaintDC dc(this);

	CRect destRect;
	CRect sourceRect;
	m_View.GetWindowRect(&destRect);
	::MapWindowPoints(HWND_DESKTOP, m_hWnd, reinterpret_cast<LPPOINT>(&destRect), 2);

	CRgn rgn;
	CPoint pnt1(destRect.left, destRect.top);
	CPoint pnt2(destRect.right, destRect.bottom);
	dc.LPtoDP(&pnt1, 1);
	dc.LPtoDP(&pnt2, 1);
	rgn.CreateRectRgn(pnt1.x, pnt1.y, pnt2.x, pnt2.y);
	dc.SelectClipRgn(&rgn);

	float zoom = (100.0f / m_Zoom.GetPos());
	int width = (int)(destRect.Width() * zoom);
	int height = (int)(destRect.Height() * zoom);

	sourceRect.left = m_Right.GetScrollPos();
	sourceRect.top = 512 - height - m_Up.GetScrollPos();
	sourceRect.right = sourceRect.left + width;
	sourceRect.bottom = sourceRect.top + height;
	m_Dib.Render(dc.GetSafeHdc(), destRect, sourceRect);

	m_Right.SetScrollRange(0, 512 - width);
	m_Up.SetScrollRange(0, 512 - height);

	CPen pen(PS_DOT, 1, RGB(192, 192, 192));
	dc.SelectObject(pen);

	if (m_ShowRect)
	{
		int len = m_Chars.GetSize();
		for (int i=0; i<len; i++)
		{
			const cCharacter& ch = m_Chars[i];

			CRect rect;
			rect.left	= ch.m_Pos.x - m_Right.GetScrollPos();
			rect.top	= ch.m_Pos.y - m_Up.GetScrollPos();
			rect.right	= rect.left + ch.m_Size.cx;
			rect.bottom = rect.top + ch.m_Size.cy;

			rect.left	= (long)(rect.left / zoom);
			rect.top	= (long)(rect.top / zoom);
			rect.right	= (long)(rect.right / zoom);
			rect.bottom = (long)(rect.bottom / zoom);

			::MapWindowPoints(m_View.GetSafeHwnd(), m_hWnd, reinterpret_cast<LPPOINT>(&rect), 2);
			rect.OffsetRect(-1, -1);

			dc.MoveTo(rect.left, rect.top);
			dc.LineTo(rect.right, rect.top);
			dc.LineTo(rect.right, rect.bottom);
			dc.LineTo(rect.left, rect.bottom);
			dc.LineTo(rect.left, rect.top);
		}
	}
}


void CMudFontDlg::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	CDialog::OnVScroll(nSBCode, nPos, pScrollBar);

	if (pScrollBar != NULL)
	{
		// Get the minimum and maximum scroll-bar positions.
		int minpos;
		int maxpos;
		pScrollBar->GetScrollRange(&minpos, &maxpos);
		maxpos = pScrollBar->GetScrollLimit();
		
		// Get the current position of scroll box.
		int curpos = pScrollBar->GetScrollPos();
		
		// Determine the new position of scroll box.
		switch (nSBCode)
		{
		case SB_TOP:
			curpos = minpos;
			break;
			
		case SB_BOTTOM:
			curpos = maxpos;
			break;
			
		case SB_ENDSCROLL:
			break;
			
		case SB_LINEUP:
			if (curpos > minpos)
				curpos--;
			break;
			
		case SB_LINEDOWN:
			if (curpos < maxpos)
				curpos++;
			break;
			
		case SB_PAGEUP:   // Scroll one page up.
			if (curpos > minpos)
			{
				curpos = max(minpos, curpos - 10);
			}
			break;
			
		case SB_PAGEDOWN:    // Scroll one page down.
			if (curpos < maxpos)
			{
				curpos = min(maxpos, curpos + 10);
			}
			break;
			
		case SB_THUMBPOSITION:
			curpos = nPos; 
			break;
			
		case SB_THUMBTRACK:
			curpos = nPos;
			break;
		}
		
		// Set the new position of the thumb (scroll box).
		pScrollBar->SetScrollPos(curpos);
		InvalidateView();
	}
}


void CMudFontDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
	if (pScrollBar != NULL)
	{
		if (pScrollBar->GetSafeHwnd() == m_Zoom.GetSafeHwnd())
		{
			int pos = m_Zoom.GetPos();
			m_ZoomText.Format(_T("%d%%"), pos);
			UpdateData(FALSE);	
			InvalidateView();
		} else
		{
			// Get the minimum and maximum scroll-bar positions.
			int minpos;
			int maxpos;
			pScrollBar->GetScrollRange(&minpos, &maxpos);
			maxpos = pScrollBar->GetScrollLimit();
			
			// Get the current position of scroll box.
			int curpos = pScrollBar->GetScrollPos();
			
			// Determine the new position of scroll box.
			switch (nSBCode)
			{
			case SB_LEFT:    // Scroll to far left.
				curpos = minpos;
				break;
				
			case SB_RIGHT:    // Scroll to far right.
				curpos = maxpos;
				break;
				
			case SB_ENDSCROLL:  // End scroll.
				break;
				
			case SB_LINELEFT:    // Scroll left.
				if (curpos > minpos)
					curpos--;
				break;
				
			case SB_LINERIGHT:  // Scroll right.
				if (curpos < maxpos)
					curpos++;
				break;
				
			case SB_PAGELEFT:   // Scroll one page left.
				if (curpos > minpos)
				{
					curpos = max(minpos, curpos - 10);
				}
				break;
				
			case SB_PAGERIGHT:    // Scroll one page right.
				if (curpos < maxpos)
				{
					curpos = min(maxpos, curpos + 10);
				}
				break;
				
			case SB_THUMBPOSITION: // Scroll to absolute position. nPos is the position
				curpos = nPos;      // of the scroll box at the end of the drag
				// operation.
				break;
				
			case SB_THUMBTRACK:   // Drag scroll box to specified position. nPos is
				// the
				curpos = nPos;     // position that the scroll box has been dragged
				// to.
				break;
			}
			
			// Set the new position of the thumb (scroll box).
			pScrollBar->SetScrollPos(curpos);
			InvalidateView();
		}
	}
}


void CMudFontDlg::SetupFont()
{
	int		index;
	int		nHeight;
	int		nWeight;
	int		nQuality;
	CString Facename;

	UpdateData(TRUE);
	m_FontName.GetWindowText(Facename);

	nHeight = m_SizeSpin.GetPos();
	if (nHeight > 100)	nHeight = 100;
	if (nHeight < 5)	nHeight = 5;
	index = m_Weight.GetCurSel();
	nWeight = m_Weight.GetItemData(index);

	index = m_Quality.GetCurSel();
	nQuality = m_Quality.GetItemData(index);

	index = m_AA.GetCurSel();
	nHeight *= m_AA.GetItemData(index);

	// delete old font.
	if (m_DrawFont.GetSafeHandle() != NULL)
	{
		m_DrawFont.DeleteObject();
	}

	// create new font.
	CPaintDC dc(this);
	int realHeight = -MulDiv(nHeight, GetDeviceCaps(dc.GetSafeHdc(), LOGPIXELSY), 72);
	m_DrawFont.CreateFont(realHeight, 0, 0, 0, nWeight, m_Italic, FALSE,
		0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		nQuality, DEFAULT_PITCH | FF_SWISS, Facename);
}


void CMudFontDlg::UpdateTextureSize()
{
	for (int y=0; y<512; y++)
	{
		for (int x=0; x<512; x++)
		{
			if (x < m_TextureWidth && y < m_TextureHeight)
			{
				m_Dib.SetPixel(x,y, RGB(0,0,0));
			} else
			{
				if ((x % 10) == (y % 10) || ((512-x) % 10) == (y % 10))
				{
					m_Dib.SetPixel(x,y, RGB(0,0,0));
				} else
				{
					m_Dib.SetPixel(x,y, RGB(128, 128, 128));
				}
			}
		}
	}
}


void CMudFontDlg::UpdateTexture()
{
	// clear the texture.
	UpdateTextureSize();
	UpdateSizes();
	UpdatePositions();

	DrawChars();
	InvalidateView();
}


void CMudFontDlg::UpdateSizes()
{
	m_MemDC.SelectObject(m_DrawFont);

	int sel = m_CharSel.GetCurSel();
	m_CharSel.ResetContent();

	int index = m_AA.GetCurSel();
	int sampling = m_AA.GetItemData(index);

	ABC abc;
	int len = m_Chars.GetSize();
	for (int i=0; i<len; i++)
	{
		cCharacter& ch = m_Chars[i];
		m_CharSel.AddString(CString(ch.m_Char));
		ch.m_Size = m_MemDC.GetTextExtent(&ch.m_Char, 1);
		m_MemDC.GetCharABCWidths(ch.m_Char, ch.m_Char, &abc);
		ch.m_Size.cx = labs(abc.abcB);
		
		ch.m_Size.cx /= sampling;
		ch.m_Size.cy /= sampling;
		ch.m_Size.cx += m_Border.left + m_Border.right + ch.m_bLeft + ch.m_bRight;
		ch.m_Size.cy += m_Border.top + m_Border.bottom;
	}

	m_CharSel.SetCurSel(sel);
}


void CMudFontDlg::UpdatePositions()
{
	int cx = 0;
	int cy = 0;
	int height = 0;

	int len = m_Chars.GetSize();
	for (int i=0; i<len; i++)
	{
		cCharacter& ch = m_Chars[i];

		if (ch.m_Size.cy > height)
		{
			height = ch.m_Size.cy;
		}

		if (cx + ch.m_Size.cx > m_TextureWidth)
		{
			cx = 0;
			cy += height + 1;
		}

		ch.m_Pos.x = cx;
		ch.m_Pos.y = cy;

		cx += ch.m_Size.cx + 1;		
	}
}


void CMudFontDlg::InvalidateView()
{
	CRect rect;
	m_View.GetWindowRect(&rect);
	::MapWindowPoints(HWND_DESKTOP, m_hWnd, reinterpret_cast<LPPOINT>(&rect), 2);
	InvalidateRect(&rect, FALSE);
}



void CMudFontDlg::OnSizeChange() 
{
	UpdateData(TRUE);

	switch (m_Width)
	{
	case 0:	m_TextureWidth = 64;	break;
	case 1:	m_TextureWidth = 128;	break;
	case 2:	m_TextureWidth = 256;	break;
	case 3:	m_TextureWidth = 512;	break;
	}

	switch (m_Height)
	{
	case 0:	m_TextureHeight = 64;	break;
	case 1:	m_TextureHeight = 128;	break;
	case 2:	m_TextureHeight = 256;	break;
	case 3:	m_TextureHeight = 512;	break;
	}

	UpdateTexture();
}


void CMudFontDlg::DrawChars()
{
	CBrush brush(RGB(0,0,0));
	CPen   pen(PS_SOLID, 1, RGB(0,0,0));
	m_MemDC.SelectObject(brush);
	m_MemDC.SelectObject(pen);
	m_MemDC.SetTextColor(RGB(255,255,255));
	m_MemDC.SetBkMode(TRANSPARENT);
	m_MemDC.SelectObject(m_DrawFont);

	//-------------------
	//public virtual COLORREF SetBkColor( COLORREF crColor );
	//m_MemDC.SetBkMode(OPAQUE);
	//m_MemDC.SetBkColor(RGB(0,0,0));
	
	//-------------------

	int index = m_AA.GetCurSel();
	int sampling = m_AA.GetItemData(index);

	ABC abc;
	int len = m_Chars.GetSize();
	for (int i=0; i<len; i++)
	{
		const cCharacter& ch = m_Chars[i];
		int width  = ch.m_Size.cx;
		int height = ch.m_Size.cy;
		int posx   = (m_Border.left + ch.m_bLeft) * sampling;
		int posy   = (m_Border.top) * sampling;
		int value, x, y;

		m_MemDC.GetCharABCWidths(ch.m_Char, ch.m_Char, &abc);

		// draw character.
		CRect rect(0, 0, width * sampling, height * sampling);
		m_MemDC.Rectangle(rect);
		m_MemDC.TextOut(posx - abc.abcA, posy, ch.m_Char);

		// quite a quick hack this way...
		// I guess we can think of a more generic way...
		switch (sampling)
		{
		case 1:
			for (y=0; y<height; y++)
			{
				for (x=0; x<width; x++)
				{
					COLORREF col = m_MemDib.GetPixel(x, y);
					if (y + ch.m_Pos.y < m_TextureHeight && x + ch.m_Pos.x < m_TextureWidth)
					{
						m_Dib.SetPixel(x + ch.m_Pos.x, y + ch.m_Pos.y, col);
					}
				}
			}
			break;
		case 2:	
			for (y=0; y<height; y++)
			{
				for (x=0; x<width; x++)
				{
					if (y + ch.m_Pos.y < m_TextureHeight && x + ch.m_Pos.x < m_TextureWidth)
					{
						int xb = x * 2;
						int yb = y * 2;
						value = m_MemDib.GetPixel(xb+0, yb+0) & 0xff;
						value += m_MemDib.GetPixel(xb+1, yb+0) & 0xff;
						value += m_MemDib.GetPixel(xb+1, yb+1) & 0xff;
						value += m_MemDib.GetPixel(xb+0, yb+1) & 0xff;
						value /= 4;

						m_Dib.SetPixel(x + ch.m_Pos.x, y + ch.m_Pos.y, RGB(value, value, value));
					}
				}
			}
			break;

		case 3:	
			for (y=0; y<height; y++)
			{
				for (x=0; x<width; x++)
				{
					if (y + ch.m_Pos.y < m_TextureHeight && x + ch.m_Pos.x < m_TextureWidth)
					{
						int xb = x * 3;
						int yb = y * 3;
						value  = (m_MemDib.GetPixel(xb+0, yb+0) & 0xff) * 1;
						value += (m_MemDib.GetPixel(xb+1, yb+0) & 0xff) * 3;
						value += (m_MemDib.GetPixel(xb+2, yb+0) & 0xff) * 1;
						value += (m_MemDib.GetPixel(xb+0, yb+1) & 0xff) * 3;
						value += (m_MemDib.GetPixel(xb+1, yb+1) & 0xff) * 5;
						value += (m_MemDib.GetPixel(xb+2, yb+1) & 0xff) * 3;
						value += (m_MemDib.GetPixel(xb+0, yb+2) & 0xff) * 1;
						value += (m_MemDib.GetPixel(xb+1, yb+2) & 0xff) * 3;
						value += (m_MemDib.GetPixel(xb+2, yb+2) & 0xff) * 1;
						value /= 21;

						m_Dib.SetPixel(x + ch.m_Pos.x, y + ch.m_Pos.y, RGB(value, value, value));
					}
				}
			}
			break;

		case 4:	
			for (y=0; y<height; y++)
			{
				for (x=0; x<width; x++)
				{
					if (y + ch.m_Pos.y < m_TextureHeight && x + ch.m_Pos.x < m_TextureWidth)
					{
						int xb = x * 4;
						int yb = y * 4;
						value  = (m_MemDib.GetPixel(xb+0, yb+0) & 0xff) * 1;
						value += (m_MemDib.GetPixel(xb+1, yb+0) & 0xff) * 2;
						value += (m_MemDib.GetPixel(xb+2, yb+0) & 0xff) * 2;
						value += (m_MemDib.GetPixel(xb+3, yb+0) & 0xff) * 1;
						value += (m_MemDib.GetPixel(xb+0, yb+1) & 0xff) * 2;
						value += (m_MemDib.GetPixel(xb+1, yb+1) & 0xff) * 4;
						value += (m_MemDib.GetPixel(xb+2, yb+1) & 0xff) * 4;
						value += (m_MemDib.GetPixel(xb+3, yb+1) & 0xff) * 2;
						value += (m_MemDib.GetPixel(xb+0, yb+2) & 0xff) * 2;
						value += (m_MemDib.GetPixel(xb+1, yb+2) & 0xff) * 4;
						value += (m_MemDib.GetPixel(xb+2, yb+2) & 0xff) * 4;
						value += (m_MemDib.GetPixel(xb+3, yb+2) & 0xff) * 2;
						value += (m_MemDib.GetPixel(xb+0, yb+3) & 0xff) * 1;
						value += (m_MemDib.GetPixel(xb+1, yb+3) & 0xff) * 2;
						value += (m_MemDib.GetPixel(xb+2, yb+3) & 0xff) * 2;
						value += (m_MemDib.GetPixel(xb+3, yb+3) & 0xff) * 1;
						value /= 36;

						m_Dib.SetPixel(x + ch.m_Pos.x, y + ch.m_Pos.y, RGB(value, value, value));
					}
				}
			}
			break;
		}

	}
}


void CMudFontDlg::AddCharacter(_TCHAR a_Char)
{
	int pos;
	int len = m_Chars.GetSize();

	// we assume the m_Char array to be sorted
	// (FIXME: force sort when loading an .fnt!)
	for (pos=0; pos<len; pos++)
	{
		if (m_Chars[pos].m_Char == a_Char)
			return;
		if (m_Chars[pos].m_Char > a_Char)
			break;
	}

	cCharacter ch;
	ch.m_Char = a_Char;
	ch.m_bLeft = 0;
	ch.m_bRight = 0;

	m_Chars.InsertAt(pos, ch);
}


int CALLBACK CMudFontDlg::EnumFontsProc(CONST LOGFONT* lplf, CONST TEXTMETRIC* lptm, DWORD dwType, LPARAM lpData)
{
	CComboBox* fonts = (CComboBox*)lpData;
	fonts->AddString(lplf->lfFaceName);
    return 1;
}


void CMudFontDlg::OnFontChange() 
{
	m_Border.left	= m_blSpin.GetPos();
	m_Border.right	= m_brSpin.GetPos();
	m_Border.top	= m_btSpin.GetPos();
	m_Border.bottom = m_bbSpin.GetPos();

	SetupFont();
	if (m_Running)
	{
		UpdateTexture();
	}
}


void CMudFontDlg::OnRectChange() 
{
	UpdateData(TRUE);
	InvalidateView();	
}


void CMudFontDlg::OnCharChange() 
{
	int index = m_CharSel.GetCurSel();
	if (index >= 0)
	{
		m_Chars[index].m_bLeft  = m_cblSpin.GetPos();
		m_Chars[index].m_bRight = m_cbrSpin.GetPos();

		if (m_Running)
		{
			UpdateTexture();
		}		
	}
}


void CMudFontDlg::OnSelchangeChar() 
{
	int index = m_CharSel.GetCurSel();
	if (index >= 0)
	{
		m_cblSpin.SetPos(m_Chars[index].m_bLeft);
		m_cbrSpin.SetPos(m_Chars[index].m_bRight);	
	}
}


void CMudFontDlg::OnToolsAddchar() 
{
	CAddCharDlg dlg;
	if (dlg.DoModal())
	{
		CString chars = dlg.GetString();
		int len = chars.GetLength();
		for (int i=0; i<len; i++)
		{
			AddCharacter(chars[i]);
		}
		UpdateTexture();
	}	
}

// wsw : jal : Warsow charset
void CMudFontDlg::OnToolsAddWesternCharset() 
{
#ifndef UNICODE
	for (int i = 32; i < 256; i++)
		AddCharacter( (_TCHAR)i );
#else
	for (int i = 32; i < 128; i++)
		AddCharacter( i );
	for (int i = 0xA0; i < 0x100; i++)
		AddCharacter( i );
#endif

	UpdateTexture();
}


void CMudFontDlg::OnToolsAddLatinA()
{
#ifdef UNICODE
	for (int i = 0x100; i < 0x200; i++)
		AddCharacter( i );
	UpdateTexture();
#endif
}

void CMudFontDlg::OnToolsAddCyrillic()
{
#ifdef UNICODE
	// Don't add all, only the most commonly used ones
	// (These should cover Russian, Belarusian, Ukrainian and Bulgarian)
	AddCharacter( 0x401 );
	AddCharacter( 0x404 );
	AddCharacter( 0x406 );
	AddCharacter( 0x407 );
	AddCharacter( 0x40e );
	for (int i = 0x410; i < 0x450; i++)
		AddCharacter( i );
	AddCharacter( 0x451 );
	AddCharacter( 0x454 );
	AddCharacter( 0x456 );
	AddCharacter( 0x457 );
	AddCharacter( 0x45e );
	UpdateTexture();
#endif
}

void CMudFontDlg::OnFileExit() 
{
	EndDialog(0);	
}


void CMudFontDlg::OnFileExport() 
{
	OPENFILENAME ofn;       // common dialog box structure
	_TCHAR szFile[260];       // buffer for file name
	memset(szFile, 0, sizeof(szFile));

	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = GetSafeHwnd();
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile)/sizeof(szFile[0]);
	ofn.lpstrFilter = _T("Targa image (*.tga)\0*.tga\0");
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrDefExt = _T("tga");
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

	// Display the Open dialog box. 
	if (GetSaveFileName(&ofn) == TRUE)
	{
		ExportTGA(szFile);
		ExportXML(szFile);
		ExportWSW(szFile); // wsw : jal : warsow font script
	}
}


void CMudFontDlg::OnFileNew() 
{
	m_Chars.RemoveAll();
	UpdateTexture();	
}


void CMudFontDlg::OnFileOpen() 
{
	OPENFILENAME ofn;       // common dialog box structure
	_TCHAR szFile[260];       // buffer for file name
	memset(szFile, 0, sizeof(szFile));

	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = GetSafeHwnd();
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile)/sizeof(szFile[0]);
	ofn.lpstrFilter = _T("Font (*.fnt)\0*.fnt\0All (*.*)\0*.*\0");
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrDefExt = _T("fnt");
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	// Display the Open dialog box. 
	if (GetOpenFileName(&ofn) == TRUE)
	{
		m_Chars.RemoveAll();
		FILE* file = _tfopen(szFile, _T("rb"));
		if (file != NULL)
		{
			int		nHeight;
			int		nWeight;
			int		nQuality;
			int		nSampling;
			CString Facename;

			fread(&m_TextureWidth, sizeof(int), 1, file);
			fread(&m_TextureHeight, sizeof(int), 1, file);

			fread(&m_Border.left, sizeof(int), 1, file);
			m_blSpin.SetPos(m_Border.left);
			fread(&m_Border.top, sizeof(int), 1, file);
			m_btSpin.SetPos(m_Border.top);
			fread(&m_Border.right, sizeof(int), 1, file);
			m_brSpin.SetPos(m_Border.right);
			fread(&m_Border.bottom, sizeof(int), 1, file);
			m_bbSpin.SetPos(m_Border.bottom);

			fread(&nHeight, sizeof(int), 1, file);
			fread(&nWeight, sizeof(int), 1, file);
			fread(&nQuality, sizeof(int), 1, file);
			fread(&nSampling, sizeof(int), 1, file);
			fread(&m_Italic, sizeof(BOOL), 1, file);

			int len;
			char buf[256];
			fread(&len, sizeof(int), 1, file);
			fread(buf, len+1, 1, file);
			Facename = buf;

			UpdateUI(nHeight, nWeight, nQuality, nSampling, Facename);
						
			int numChars;
			fread(&numChars, sizeof(int), 1, file);
			for (int i=0; i<numChars; i++)
			{
				char ch;
				int bl, br;

				fread(&ch, sizeof(char), 1, file);
				fread(&bl, sizeof(int), 1, file);
				fread(&br, sizeof(int), 1, file);

				cCharacter kar;
				kar.m_Char = ch;
				kar.m_bLeft = bl;
				kar.m_bRight = br;
				m_Chars.Add(kar);
			}

			fclose(file);
			OnFontChange();
		}
	}	
	DWORD error = CommDlgExtendedError();

}


void CMudFontDlg::OnFileSave() 
{
	OPENFILENAME ofn;       // common dialog box structure
	_TCHAR szFile[260];       // buffer for file name
	memset(szFile, 0, sizeof(szFile));

	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = GetSafeHwnd();
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile)/sizeof(szFile[0]);
	ofn.lpstrFilter = _T("Font (*.fnt)\0*.fnt\0All (*.*)\0*.*\0");
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrDefExt = _T("fnt");
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

	// Display the Open dialog box. 
	if (GetSaveFileName(&ofn) == TRUE)
	{
		int		index;
		int		nHeight;
		int		nWeight;
		int		nQuality;
		int		nSampling;
		CString Facename;

		UpdateData(TRUE);
		m_FontName.GetWindowText(Facename);

		nHeight = m_SizeSpin.GetPos();
		if (nHeight > 100)	nHeight = 100;
		if (nHeight < 5)	nHeight = 5;
		index = m_Weight.GetCurSel();
		nWeight = m_Weight.GetItemData(index);

		index = m_Quality.GetCurSel();
		nQuality = m_Quality.GetItemData(index);

		index = m_AA.GetCurSel();
		nSampling = m_AA.GetItemData(index);

		FILE* file = _tfopen(szFile, _T("w+b"));
		if (file != NULL)
		{
			fwrite(&m_TextureWidth, sizeof(int), 1, file);
			fwrite(&m_TextureHeight, sizeof(int), 1, file);
			fwrite(&m_Border.left, sizeof(int), 1, file);
			fwrite(&m_Border.top, sizeof(int), 1, file);
			fwrite(&m_Border.right, sizeof(int), 1, file);
			fwrite(&m_Border.bottom, sizeof(int), 1, file);

			fwrite(&nHeight, sizeof(int), 1, file);
			fwrite(&nWeight, sizeof(int), 1, file);
			fwrite(&nQuality, sizeof(int), 1, file);
			fwrite(&nSampling, sizeof(int), 1, file);
			fwrite(&m_Italic, sizeof(BOOL), 1, file);

			int len = Facename.GetLength();
			fwrite(&len, sizeof(int), 1, file);
			fwrite((LPCTSTR)Facename, len+1, 1, file);
						
			int numChars = m_Chars.GetSize();
			fwrite(&numChars, sizeof(int), 1, file);
			for (int i=0; i<numChars; i++)
			{
				fwrite(&m_Chars[i].m_Char, sizeof(char), 1, file);
				fwrite(&m_Chars[i].m_bLeft, sizeof(int), 1, file);
				fwrite(&m_Chars[i].m_bRight, sizeof(int), 1, file);
			}

			fclose(file);
		}
	}	
	DWORD error = CommDlgExtendedError();
	
}


void CMudFontDlg::UpdateUI(int nHeight, int nWeight, int nQuality, int nSampling, const CString& Facename)
{
	int i, len;

	UpdateData(TRUE);
	m_SizeSpin.SetPos(nHeight);

	len = m_Weight.GetCount();
	for (i=0; i<len; i++)
	{
		if ((int)m_Weight.GetItemData(i) == nWeight)
		{
			m_Weight.SetCurSel(i);
			break;
		}
	}

	len = m_Quality.GetCount();
	for (i=0; i<len; i++)
	{
		if ((int)m_Quality.GetItemData(i) == nQuality)
		{
			m_Quality.SetCurSel(i);
			break;
		}
	}

	len = m_AA.GetCount();
	for (i=0; i<len; i++)
	{
		if ((int)m_AA.GetItemData(i) == nSampling)
		{
			m_AA.SetCurSel(i);
			break;
		}
	}

	len = m_FontName.GetCount();
	for (i=0; i<len; i++)
	{
		CString str;
		m_FontName.GetLBText(i, str);
		if (str.CompareNoCase(Facename) == 0)
		{
			m_FontName.SetCurSel(i);
			break;
		}
	}
}



void CMudFontDlg::ExportTGA(const _TCHAR* a_Filename)
{
	_TCHAR path[_MAX_PATH];
	_TCHAR drive[_MAX_DRIVE];
	_TCHAR dir[_MAX_DIR];
	_TCHAR fname[_MAX_FNAME];
	_tsplitpath(a_Filename, drive, dir, fname, NULL);
	_tmakepath(path, drive, dir, fname, _T(".tga"));

	FILE* file = _tfopen(path, _T("w+b"));
	if (file == NULL) return;

	char	Id_Size			= 0;			// no user data at end of tga.
	char	ColorMap_Type	= 0;			// no colormap.
	char	Image_Type      = 2;			// uncompressed raw image data. (RGBA)
	short	ColorMap_Low	= 0;			// no color map, so this must be set to zero.
	short	ColorMap_Length	= 0;			// no color map, so this must be set to zero.
	char	ColorMap_Bits	= 0;			// no color map, so this must be set to zero.
	short	XOrigin			= 0;			// ??
	short	YOrigin			= 0;			// ??
	short	Width			= m_TextureWidth;
	short	Height			= m_TextureHeight;
	char	BPP				= 32;			// 32bits RGBA.
	char	AttrBits		= 8;			// 8 bits for alpha.

	fwrite(&Id_Size, sizeof(char), 1, file);
	fwrite(&ColorMap_Type, sizeof(char), 1, file);
	fwrite(&Image_Type, sizeof(char), 1, file);
	fwrite(&ColorMap_Low, sizeof(short), 1, file);
	fwrite(&ColorMap_Length, sizeof(short), 1, file);
	fwrite(&ColorMap_Bits, sizeof(char), 1, file);	
	fwrite(&XOrigin, sizeof(short), 1, file);
	fwrite(&YOrigin, sizeof(short), 1, file);
	fwrite(&Width, sizeof(short), 1, file);
	fwrite(&Height, sizeof(short), 1, file);
	fwrite(&BPP, sizeof(char), 1, file);		
	fwrite(&AttrBits, sizeof(char), 1, file);	

	// flip the image in vertical
	for (int y=m_TextureHeight; y>=0; y--)
	{
		for (int x=0; x<m_TextureWidth; x++)
		{
			// get intensity.
			int intensity = m_Dib.GetPixel(x,y) & 0xff;

			// write rgba color;
			unsigned int color = intensity << 24 | 0x00ffffff;
			fwrite(&color, sizeof(unsigned int), 1, file);
		}
	}

	fclose(file);
}


void CMudFontDlg::ExportXML(const _TCHAR* a_Filename)
{
// wsw : jal : we don't use the xml description
/*
	char path[_MAX_PATH];
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	_splitpath(a_Filename, drive, dir, fname, NULL);
	_makepath(path, drive, dir, fname, ".xml");

	FILE* file = fopen(path, "w+t");
	if (file == NULL) return;

	fprintf(file, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
	fprintf(file, "<font>\n");

	int index = m_AA.GetCurSel();
	int sampling = m_AA.GetItemData(index);

	ABC abc;
	m_MemDC.SelectObject(m_DrawFont);
	int numChars = m_Chars.GetSize();
	for (int i=0; i<numChars; i++)
	{
		const cCharacter& ch = m_Chars[i];
		fprintf(file, "\t<char id=\"%d\" x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" />\n",
			ch.m_Char, ch.m_Pos.x, ch.m_Pos.y, ch.m_Size.cx, ch.m_Size.cy);

		m_MemDC.GetCharABCWidths(ch.m_Char, ch.m_Char, &abc);
		abc.abcA /= sampling;
		abc.abcB /= sampling;
		abc.abcC /= sampling;

		abc.abcA -= (m_Border.left + ch.m_bLeft);
		abc.abcB += (m_Border.left + ch.m_bLeft) + (m_Border.right + ch.m_bRight);
		abc.abcC -= (m_Border.right + ch.m_bRight);

		fprintf(file, "\t<spacing id=\"%d\" a=\"%d\" b=\"%d\" c=\"%d\" />\n",
			ch.m_Char, abc.abcA, abc.abcB, abc.abcC);
	}

	fprintf(file, "</font>\n");
	fclose(file);
*/
}


 // wsw : jal : warsow font
void CMudFontDlg::ExportWSW(const _TCHAR* a_Filename)
{
	_TCHAR path[_MAX_PATH];
	_TCHAR drive[_MAX_DRIVE];
	_TCHAR dir[_MAX_DIR];
	_TCHAR fname[_MAX_FNAME];
	_tsplitpath(a_Filename, drive, dir, fname, NULL);
	_tmakepath(path, drive, dir, fname, _T(".wfd"));

	FILE* file = _tfopen(path, _T("w+t"));
	if (file == NULL) return;

	fprintf(file, "// WARSOW Mudfont. version=\"1.1\" encoding=\"UTF-8\"\n");
	fprintf(file, "// \"<texture width>\" \"<texture height>\"\n");

	// wsw : write tga image size
	fprintf(file, "%d %d\n", m_TextureWidth, m_TextureHeight);

	// format description
	fprintf(file, "// \"<char>\" \"<x>\" \"<y>\" \"<width>\" \"<height>\"\n" );

	int index = m_AA.GetCurSel();
	int sampling = m_AA.GetItemData(index);

	//ABC abc;
	m_MemDC.SelectObject(m_DrawFont);
	int numChars = m_Chars.GetSize();
	for (int i=0; i<numChars; i++)
	{
		const cCharacter& ch = m_Chars[i];

#ifndef UNICODE
		fprintf(file, "%d %d %d %d %d\n",
			((byte)ch.m_Char), ch.m_Pos.x, ch.m_Pos.y, ch.m_Size.cx, ch.m_Size.cy);
#else
		fprintf(file, "%d %d %d %d %d\n",
			ch.m_Char, ch.m_Pos.x, ch.m_Pos.y, ch.m_Size.cx, ch.m_Size.cy);
#endif
	}

	fprintf(file, "end\n");
	fclose(file);
}

// --> End of file <-----------------------------------------------------------

