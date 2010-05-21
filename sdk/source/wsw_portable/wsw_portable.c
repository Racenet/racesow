#include <windows.h>

#include "../game/config.h"

#define LAPP_EXECUTABLE L"Warsow"

#ifdef _M_IX86
#define LARCH L"_x86"
#elif defined ( _M_AMD64 )
#define LARCH L"_x64"
#else
#define LARCH ""
#endif

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
    SHELLEXECUTEINFO sei = { sizeof(sei) };
	LPWSTR newWCmdLine;
	LPCSTR preCmdLine = "+set fs_usehomedir 0 ";
	LPSTR newCmdLine;
	const size_t preCmdLineLen = strlen( preCmdLine );
	size_t newCmdLineSize;

	newCmdLineSize = preCmdLineLen + strlen( lpCmdLine ) + 1;
	newCmdLine = (LPSTR)malloc( newCmdLineSize );
	memcpy( newCmdLine, preCmdLine, preCmdLineLen );
	memcpy( newCmdLine + preCmdLineLen, lpCmdLine, newCmdLineSize - preCmdLineLen );
	newCmdLine[newCmdLineSize-1] = '\0';

	newWCmdLine = (LPWSTR)malloc( newCmdLineSize * sizeof (WCHAR) );
	MultiByteToWideChar( CP_ACP, 0, newCmdLine, -1, newWCmdLine, (int)newCmdLineSize );

	sei.fMask = SEE_MASK_FLAG_DDEWAIT|SEE_MASK_FLAG_NO_UI;
	sei.nShow = SW_SHOWNORMAL;
	sei.lpVerb = L"open";
	sei.lpFile = LAPP_EXECUTABLE LARCH L".exe";
	sei.lpParameters = newWCmdLine;

	if( ShellExecuteEx( &sei ) == FALSE )
	{	
		sei.fMask &= ~SEE_MASK_FLAG_NO_UI;
		sei.nShow = SW_SHOWNORMAL;
		sei.lpVerb = L"open";
		sei.lpFile = LAPP_EXECUTABLE L".exe";
		sei.lpParameters = newWCmdLine;

		if( ShellExecuteEx( &sei ) == FALSE )
		{
		}
	}

	free( newCmdLine );
	free( newWCmdLine );

	return 0;
}
