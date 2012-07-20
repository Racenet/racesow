/*
Copyright (C) 2002-2003 Victor Luchits

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "client.h"
#include "../ui/ui_public.h"
#include "../qcommon/asyncstream.h"

// Structure containing functions exported from user interface DLL
static ui_export_t *uie;

EXTERN_API_FUNC void *GetGameAPI( void * );

static mempool_t *ui_mempool;
static void *module_handle;

static async_stream_module_t *ui_async_stream;

//==============================================

/*
* CL_UIModule_Print
*/
static void CL_UIModule_Print( const char *msg ) {
	Com_Printf( "%s", msg );
}

/*
* CL_UIModule_Error
*/
static void CL_UIModule_Error( const char *msg ) {
	Com_Error( ERR_FATAL, "%s", msg );
}

/*
* CL_UIModule_GetConfigString
*/
static void CL_UIModule_GetConfigString( int i, char *str, int size )
{
	if( i < 0 || i >= MAX_CONFIGSTRINGS )
		Com_Error( ERR_DROP, "CL_UIModule_GetConfigString: i > MAX_CONFIGSTRINGS" );
	if( !str || size <= 0 )
		Com_Error( ERR_DROP, "CL_UIModule_GetConfigString: NULL string" );

	Q_strncpyz( str, cl.configstrings[i], size );
}

/*
* CL_UIModule_MemAlloc
*/
static void *CL_UIModule_MemAlloc( size_t size, const char *filename, int fileline ) {
	return _Mem_Alloc( ui_mempool, size, MEMPOOL_USERINTERFACE, 0, filename, fileline );
}

/*
* CL_UIModule_MemFree
*/
static void CL_UIModule_MemFree( void *data, const char *filename, int fileline ) {
	_Mem_Free( data, MEMPOOL_USERINTERFACE, 0, filename, fileline );
}

//==============================================


/*
* CL_UIModule_AsyncStream_Init
*/
static void CL_UIModule_AsyncStream_Init( void )
{
	ui_async_stream = AsyncStream_InitModule( "UI", CL_UIModule_MemAlloc, CL_UIModule_MemFree );
}

/*
* CL_UIModule_AsyncStream_PerformRequest
*/
static int CL_UIModule_AsyncStream_PerformRequest( const char *url, const char *method, const char *data, int timeout,
	ui_async_stream_read_cb_t read_cb, ui_async_stream_done_cb_t done_cb, void *privatep )
{
	assert( ui_async_stream );
	return AsyncStream_PerformRequest( ui_async_stream, url, method, data, NULL, timeout, 0, read_cb, done_cb, privatep );
}

/*
* CL_UIModule_AsyncStream_Shutdown
*/
static void CL_UIModule_AsyncStream_Shutdown( void )
{
	AsyncStream_ShutdownModule( ui_async_stream );
	ui_async_stream = NULL;
}

/*
* CL_UIModule_R_RegisterWorldModel
*/
static void CL_UIModule_R_RegisterWorldModel( const char *model ) {
	R_RegisterWorldModel( model, NULL );
}

//==============================================

/*
* CL_UIModule_Init
*/
void CL_UIModule_Init( void )
{
	int apiversion;
	ui_import_t import;
	void *( *builtinAPIfunc )(void *) = NULL;
#ifdef UI_HARD_LINKED
	builtinAPIfunc = GetUIAPI;
#endif

	CL_UIModule_Shutdown();

	ui_mempool = _Mem_AllocPool( NULL, "User Iterface", MEMPOOL_USERINTERFACE, __FILE__, __LINE__ );

	import.Error = CL_UIModule_Error;
	import.Print = CL_UIModule_Print;

	import.Dynvar_Create = Dynvar_Create;
	import.Dynvar_Destroy = Dynvar_Destroy;
	import.Dynvar_Lookup = Dynvar_Lookup;
	import.Dynvar_GetName = Dynvar_GetName;
	import.Dynvar_GetValue = Dynvar_GetValue;
	import.Dynvar_SetValue = Dynvar_SetValue;
	import.Dynvar_AddListener = Dynvar_AddListener;
	import.Dynvar_RemoveListener = Dynvar_RemoveListener;

	import.Cvar_Get = Cvar_Get;
	import.Cvar_Set = Cvar_Set;
	import.Cvar_SetValue = Cvar_SetValue;
	import.Cvar_ForceSet = Cvar_ForceSet;
	import.Cvar_String = Cvar_String;
	import.Cvar_Value = Cvar_Value;

	import.Cmd_Argc = Cmd_Argc;
	import.Cmd_Argv = Cmd_Argv;
	import.Cmd_Args = Cmd_Args;

	import.Cmd_AddCommand = Cmd_AddCommand;
	import.Cmd_RemoveCommand = Cmd_RemoveCommand;
	import.Cmd_ExecuteText = Cbuf_ExecuteText;
	import.Cmd_Execute = Cbuf_Execute;
	import.Cmd_SetCompletionFunc = Cmd_SetCompletionFunc;

	import.FS_FOpenFile = FS_FOpenFile;
	import.FS_Read = FS_Read;
	import.FS_Write = FS_Write;
	import.FS_Print = FS_Print;
	import.FS_Tell = FS_Tell;
	import.FS_Seek = FS_Seek;
	import.FS_Eof = FS_Eof;
	import.FS_Flush = FS_Flush;
	import.FS_FCloseFile = FS_FCloseFile;
	import.FS_RemoveFile = FS_RemoveFile;
	import.FS_GetFileList = FS_GetFileList;
	import.FS_GetGameDirectoryList = FS_GetGameDirectoryList;
	import.FS_FirstExtension = FS_FirstExtension;
	import.FS_MoveFile = FS_MoveFile;
	import.FS_IsUrl = FS_IsUrl;
	import.FS_FileMTime = FS_FileMTime;
	import.FS_RemoveDirectory = FS_RemoveDirectory;

	import.CL_Quit = CL_Quit;
	import.CL_SetKeyDest = CL_SetKeyDest;
	import.CL_ResetServerCount = CL_ResetServerCount;
	import.CL_GetClipboardData = CL_GetClipboardData;
	import.CL_FreeClipboardData = CL_FreeClipboardData;
	import.CL_OpenURLInBrowser = CL_OpenURLInBrowser;
	import.CL_ReadDemoMetaData = CL_ReadDemoMetaData;

	import.Key_ClearStates = Key_ClearStates;
	import.Key_GetBindingBuf = Key_GetBindingBuf;
	import.Key_KeynumToString = Key_KeynumToString;
	import.Key_StringToKeynum = Key_StringToKeynum;
	import.Key_SetBinding = Key_SetBinding;
	import.Key_IsDown = Key_IsDown;

	import.R_ClearScene = R_ClearScene;
	import.R_AddEntityToScene = R_AddEntityToScene;
	import.R_AddLightToScene = R_AddLightToScene;
	import.R_AddPolyToScene = R_AddPolyToScene;
	import.R_RenderScene = R_RenderScene;
	import.R_EndFrame = R_EndFrame;
	import.R_RegisterWorldModel = CL_UIModule_R_RegisterWorldModel;
	import.R_ModelBounds = R_ModelBounds;
	import.R_ModelFrameBounds = R_ModelFrameBounds;
	import.R_RegisterModel = R_RegisterModel;
	import.R_RegisterPic = R_RegisterPic;
	import.R_RegisterRawPic = R_RegisterRawPic;
	import.R_RegisterLevelshot = R_RegisterLevelshot;
	import.R_RegisterSkin = R_RegisterSkin;
	import.R_RegisterSkinFile = R_RegisterSkinFile;
	import.R_RegisterVideo = R_RegisterVideo;
	import.R_LerpTag = R_LerpTag;
	import.R_DrawStretchPic = R_DrawStretchPic;
	import.R_DrawRotatedStretchPic = R_DrawRotatedStretchPic;
	import.R_DrawStretchPoly = R_DrawStretchPoly;
	import.R_TransformVectorToScreen = R_TransformVectorToScreen;
	import.R_SetScissorRegion = R_SetScissorRegion;
	import.R_GetShaderDimensions = R_GetShaderDimensions;
	import.R_SkeletalGetNumBones = R_SkeletalGetNumBones;
	import.R_SkeletalGetBoneInfo = R_SkeletalGetBoneInfo;
	import.R_SkeletalGetBonePose = R_SkeletalGetBonePose;
	import.S_RegisterSound = CL_SoundModule_RegisterSound;
	import.S_StartLocalSound = CL_SoundModule_StartLocalSound;
	import.S_StartBackgroundTrack = CL_SoundModule_StartBackgroundTrack;
	import.S_StopBackgroundTrack = CL_SoundModule_StopBackgroundTrack;

	import.SCR_RegisterFont = SCR_RegisterFont;
	import.SCR_DrawString = SCR_DrawString;
	import.SCR_DrawStringWidth = SCR_DrawStringWidth;
	import.SCR_DrawClampString = SCR_DrawClampString;
	import.SCR_strHeight = SCR_strHeight;
	import.SCR_strWidth = SCR_strWidth;
	import.SCR_StrlenForWidth = SCR_StrlenForWidth;

	import.GetConfigString = CL_UIModule_GetConfigString;

	import.Milliseconds = Sys_Milliseconds;
	import.Microseconds = Sys_Microseconds;

	import.Hash_BlockChecksum = Com_MD5Digest32;
	import.Hash_SuperFastHash = Com_SuperFastHash;

	import.AsyncStream_UrlEncode = AsyncStream_UrlEncode;
	import.AsyncStream_UrlDecode = AsyncStream_UrlDecode;
	import.AsyncStream_PerformRequest = CL_UIModule_AsyncStream_PerformRequest;

	import.VID_GetModeInfo = VID_GetModeInfo;
	import.VID_FlashWindow = VID_FlashWindow;

	import.Mem_Alloc = CL_UIModule_MemAlloc;
	import.Mem_Free = CL_UIModule_MemFree;

	import.ML_GetFilename = ML_GetFilename;
	import.ML_GetFullname = ML_GetFullname;
	import.ML_GetMapByNum = ML_GetMapByNum;

	import.MM_Login = CL_MM_Login;
	import.MM_Logout = CL_MM_Logout;
	import.MM_GetLoginState = CL_MM_GetLoginState;
	import.MM_GetLastErrorMessage = CL_MM_GetLastErrorMessage;
	import.MM_GetProfileURL = CL_MM_GetProfileURL;
	import.MM_GetBaseWebURL = CL_MM_GetBaseWebURL;

	import.asGetAngelExport = Com_asGetAngelExport;

	import.Irc_HistorySize = Irc_HistorySize;
	import.Irc_HistoryTotalSize = Irc_HistoryTotalSize;
	import.Irc_GetHistoryHeadNode = Irc_GetHistoryHeadNode;
	import.Irc_GetNextHistoryNode = Irc_GetNextHistoryNode;
	import.Irc_GetPrevHistoryNode = Irc_GetPrevHistoryNode;
	import.Irc_GetHistoryNodeLine = Irc_GetHistoryNodeLine;

	uie = (ui_export_t *)Com_LoadGameLibrary( "ui", "GetUIAPI", &module_handle, &import, builtinAPIfunc, cls.sv_pure, NULL );
	if( !uie )
		Com_Error( ERR_DROP, "Failed to load UI dll" );

	apiversion = uie->API();
	if( apiversion != UI_API_VERSION )
	{
		Com_UnloadGameLibrary( &module_handle );
		Mem_FreePool( &ui_mempool );
		uie = NULL;
		Com_Error( ERR_FATAL, "UI version is %i, not %i", apiversion, UI_API_VERSION );
	}

	CL_UIModule_AsyncStream_Init();

	uie->Init( viddef.width, viddef.height, APP_PROTOCOL_VERSION, cls.mediaRandomSeed, cls.demo.playing, cls.demo.name );
}

/*
* CL_UIModule_Shutdown
*/
void CL_UIModule_Shutdown( void )
{
	if( !uie )
		return;

	CL_UIModule_AsyncStream_Shutdown();

	uie->Shutdown();
	Mem_FreePool( &ui_mempool );
	Com_UnloadGameLibrary( &module_handle );
	uie = NULL;
}

/*
* CL_UIModule_Refresh
*/
void CL_UIModule_Refresh( qboolean backGround, qboolean showCursor )
{
	if( uie )
		uie->Refresh( cls.realtime, Com_ClientState(), Com_ServerState(), cls.demo.paused, Q_rint(cls.demo.time/1000.0f), backGround, showCursor );
}

/*
* CL_UIModule_UpdateConnectScreen
*/
void CL_UIModule_UpdateConnectScreen( qboolean backGround )
{
	if( uie )
	{
		int downloadType, downloadSpeed;

		if( cls.download.web )
			downloadType = DOWNLOADTYPE_WEB;
		else if( cls.download.filenum )
			downloadType = DOWNLOADTYPE_SERVER;
		else
			downloadType = DOWNLOADTYPE_NONE;

		if( downloadType )
		{
#if 0
#define DLSAMPLESCOUNT 32
#define DLSSAMPLESMASK ( DLSAMPLESCOUNT-1 )
			int i, samples;
			size_t downloadedSize;
			unsigned int downloadTime;
			static int lastFrameCount = 0, frameCount = 0;
			static unsigned int downloadSpeeds[DLSAMPLESCOUNT];
			float avDownloadSpeed;

			downloadedSize = (size_t)( cls.download.size * cls.download.percent ) - cls.download.baseoffset;
			downloadTime = Sys_Milliseconds() - cls.download.timestart;
			if( downloadTime > 200 )
			{
				downloadSpeed = ( downloadedSize / 1024.0f ) / ( downloadTime * 0.001f );

				if( cls.framecount > lastFrameCount + DLSAMPLESCOUNT )
					frameCount = 0;
				lastFrameCount = cls.framecount;

				downloadSpeeds[frameCount & DLSSAMPLESMASK] = downloadSpeed;
				frameCount = max( frameCount + 1, 1 );
				samples = min( frameCount, DLSAMPLESCOUNT );

				for( avDownloadSpeed = 0.0f, i = 0; i < samples; i++ )
					avDownloadSpeed += downloadSpeeds[i];

				avDownloadSpeed /= samples;
				downloadSpeed = (int)avDownloadSpeed;
			}
			else
			{
				lastFrameCount = -1;
				downloadSpeed = 0;
			}
#else
			size_t downloadedSize;
			unsigned int downloadTime;

			downloadedSize = (size_t)( cls.download.size * cls.download.percent ) - cls.download.baseoffset;
			downloadTime = Sys_Milliseconds() - cls.download.timestart;

			downloadSpeed = downloadTime ? ( downloadedSize / 1024.0f ) / ( downloadTime * 0.001f ) : 0;
#endif
		}
		else
		{
			downloadSpeed = 0;
		}

		uie->UpdateConnectScreen( cls.servername, cls.rejected ? cls.rejectmessage : NULL,
			downloadType, cls.download.name, cls.download.percent * 100.0f, downloadSpeed,
			cls.connect_count, backGround );

		CL_UIModule_Refresh( backGround, qfalse );	
	}
}

/*
* CL_UIModule_Keydown
*/
void CL_UIModule_Keydown( int key )
{
	if( uie )
		uie->Keydown( key );
}

/*
* CL_UIModule_Keyup
*/
void CL_UIModule_Keyup( int key )
{
	if( uie )
		uie->Keyup( key );
}

/*
* CL_UIModule_CharEvent
*/
void CL_UIModule_CharEvent( qwchar key )
{
	if( uie )
		uie->CharEvent( key );
}

/*
* CL_UIModule_ForceMenuOff
*/
void CL_UIModule_ForceMenuOff( void )
{
	if( uie )
		uie->ForceMenuOff();
}

/*
* CL_UIModule_AddToServerList
*/
void CL_UIModule_AddToServerList( const char *adr, const char *info )
{
	if( uie )
		uie->AddToServerList( adr, info );
}

/*
* CL_UIModule_MouseMove
*/
void CL_UIModule_MouseMove( int dx, int dy )
{
	if( uie )
		uie->MouseMove( dx, dy );
}

/*
* CL_UIModule_MM_UIReply
*/
void CL_UIModule_MM_UIReply( int action, const char *data )
{
}
