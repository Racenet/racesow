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
#include "MudDib.h"
#pragma hdrstop 

// --> Class implementation <--------------------------------------------------


CMudDib::CMudDib ():
	m_Bits(NULL),
	m_Bitmap(NULL),
	m_Width(0),
	m_Height(0)
{
}


CMudDib::~CMudDib()
{
	Destroy ();
}


bool CMudDib::Create(int a_Width, int a_Height, HDC a_DC)
{
	Destroy();

	ZeroMemory(&m_Info, sizeof (BITMAPINFO));
	m_Info.bmiHeader.biSize			= sizeof(BITMAPINFOHEADER);
	m_Info.bmiHeader.biWidth		= a_Width;
	m_Info.bmiHeader.biHeight		= -a_Height;
	m_Info.bmiHeader.biPlanes		= 1;
	m_Info.bmiHeader.biBitCount		= 32; 
	m_Info.bmiHeader.biCompression	= BI_RGB;
	m_Info.bmiHeader.biSizeImage	= a_Width*a_Height*4;
	
	m_Bitmap = CreateDIBSection(a_DC, &m_Info, DIB_RGB_COLORS, (void **)&m_Bits, NULL, NULL); 
	
	if (a_DC != NULL)
	{
		SelectObject(a_DC, m_Bitmap);
	}

	if (m_Bitmap)
	{
		m_Width = a_Width;
		m_Height = a_Height;
		return true;
	}
	else
	{
		m_Width = 0;
		m_Height = 0;
		return false;
	}
}


bool CMudDib::Destroy ()
{
	if (m_Bitmap)
	{
		DeleteObject(m_Bitmap);
	}

	m_Bitmap	= NULL;
	m_Width		= 0;
	m_Height	= 0;

	return true;
}


void CMudDib::Render(HDC a_DC, int a_X, int a_Y, int a_Width, int a_Height)
{
	StretchDIBits(a_DC, 
		a_X, a_Y, a_Width, a_Height,
		0,0, m_Width, m_Height, 
		m_Bits, &m_Info, DIB_RGB_COLORS, SRCCOPY);
}


void CMudDib::Render(HDC a_DC, const CRect& a_Dest, const CRect& a_Source)
{
	StretchDIBits(a_DC, 
		a_Dest.left, a_Dest.top, a_Dest.Width(), a_Dest.Height(), 
		a_Source.left, a_Source.top, a_Source.Width(), a_Source.Height(),
		m_Bits, &m_Info, DIB_RGB_COLORS, SRCCOPY);
}


static inline COLORREF gSwapRedBlue(COLORREF a_Color)
{ 
	return (a_Color & 0xff00ff00)   |
		   ((a_Color >> 16) & 0xff) |
		   ((a_Color & 0xff) << 16);
}


void CMudDib::SetPixel(int a_X, int a_Y, COLORREF a_Color)
{
	if (a_Y < 0 || a_X < 0)
		return;
	if (a_Y >= m_Height || a_X >= m_Width)
		return;

	m_Bits[a_Y * m_Width + a_X] = gSwapRedBlue(a_Color);
}


COLORREF CMudDib::GetPixel(int a_X, int a_Y)
{
	if (a_Y < 0 || a_X < 0)
		return 0;
	if (a_Y >= m_Height || a_X >= m_Width)
		return 0;

	return gSwapRedBlue(m_Bits[a_Y * m_Width + a_X]);
}


// --> End of file <-----------------------------------------------------------


