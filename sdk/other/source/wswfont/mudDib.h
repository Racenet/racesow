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
#ifndef HEADER_dib 
#define HEADER_dib

// --> Class declarations <----------------------------------------------------

class CMudDib
{
public:
				CMudDib();
	virtual		~CMudDib();

	bool		Create(int a_Width, int a_Height, HDC a_DC=NULL);
	bool		Destroy();

	void		Render(HDC a_DC, int a_X, int a_Y, int a_Width, int a_Height);
	void		Render(HDC a_DC, const CRect& a_Dest, const CRect& a_Source);

	void		SetPixel(int a_X, int a_Y, COLORREF a_Color);
	COLORREF	GetPixel(int a_X, int a_Y);

	COLORREF*	GetBits();
	COLORREF*	GetScanline(int a_Y);
	int			GetWidth() const;
	int			GetHeight() const;
	int			GetPitch() const;

private:
	COLORREF*	m_Bits;
	BITMAPINFO	m_Info;
	HBITMAP		m_Bitmap;
	int			m_Width;
	int			m_Height;
};


// --> Inline functions <------------------------------------------------------

inline COLORREF* CMudDib::GetBits()
{
	return m_Bits;
}


inline COLORREF* CMudDib::GetScanline(int a_Y)
{
	return m_Bits + (a_Y*m_Width);
}


inline int CMudDib::GetWidth() const
{
	return m_Width;
}


inline int CMudDib::GetHeight() const
{
	return m_Height;
}


inline int CMudDib::GetPitch() const
{
	return m_Width*4;
}


// --> End of file <-----------------------------------------------------------
#endif // HEADER_dib
