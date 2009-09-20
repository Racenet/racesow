
#include "qas_local.h"

angelwrap_import_t ANGELWRAP_IMPORT;

#ifdef __cplusplus
extern "C" {
#endif

angelwrap_export_t *GetAngelwrapAPI( angelwrap_import_t *import )
{
	static angelwrap_export_t globals;

	ANGELWRAP_IMPORT = *import;

	globals.API = QAS_API;
	globals.Init = QAS_Init;
	globals.Shutdown = QAS_ShutDown;

	globals.asGetAngelExport = QAS_GetAngelExport;

	return &globals;
}

/*
#if defined ( HAVE_DLLMAIN ) && !defined ( ANGELWRAP_HARD_LINKED )
int _stdcall DLLMain( void *hinstDll, unsigned long dwReason, void *reserved )
{
	return 1;
}
#endif
*/

#ifdef __cplusplus
}
#endif


