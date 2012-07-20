#ifndef __UI_SYSCALLS_H__
#define __UI_SYSCALLS_H__

#include "ui_public.h"

// in ui_public.cpp
extern "C" QF_DLL_EXPORT ui_export_t *GetUIAPI( ui_import_t *import );

namespace WSWUI {
	extern ui_import_t UI_IMPORT;
}

namespace trap
{
		using WSWUI::UI_IMPORT;

		inline void Error( const char *str ) {
			UI_IMPORT.Error( str );
		}

		inline void Print( const char *str ) {
			UI_IMPORT.Print( str );
		}

		inline void Cmd_AddCommand( const char *name, void(*cmd)(void) ) {
			UI_IMPORT.Cmd_AddCommand( name, cmd );
		}

		inline void Cmd_RemoveCommand( const char *cmd_name ) {
			UI_IMPORT.Cmd_RemoveCommand( cmd_name );
		}

		inline void Cmd_ExecuteText( int exec_when, const char *text ) {
			UI_IMPORT.Cmd_ExecuteText( exec_when, text );
		}

		inline void Cmd_Execute( void ) {
			UI_IMPORT.Cmd_Execute ();
		}

		inline void R_ClearScene( void ) {
			UI_IMPORT.R_ClearScene ();
		}

		inline void R_SetScissorRegion( int x, int y, int w, int h ) {
			UI_IMPORT.R_SetScissorRegion( x, y, w, h );
		}

		inline void R_AddEntityToScene( entity_t *ent ) {
			UI_IMPORT.R_AddEntityToScene( ent );
		}

		inline void R_AddLightToScene( vec3_t org, float intensity, float r, float g, float b, struct shader_s *shader ) {
			UI_IMPORT.R_AddLightToScene( org, intensity, r, g, b, shader );
		}

		inline void R_AddPolyToScene( poly_t *poly ) {
			UI_IMPORT.R_AddPolyToScene( poly );
		}

		inline void R_RenderScene( refdef_t *fd ) {
			UI_IMPORT.R_RenderScene( fd );
		}

		inline void R_EndFrame( void ) {
			UI_IMPORT.R_EndFrame ();
		}

		inline void R_RegisterWorldModel( const char *name ) {
			UI_IMPORT.R_RegisterWorldModel( name);
		}

		inline void R_ModelBounds( struct model_s *mod, vec3_t mins, vec3_t maxs ) {
			UI_IMPORT.R_ModelBounds( mod, mins, maxs );
		}

		inline void R_ModelFrameBounds( struct model_s *mod, int frame, vec3_t mins, vec3_t maxs ) {
			UI_IMPORT.R_ModelFrameBounds( mod, frame, mins, maxs );
		}

		inline struct model_s *R_RegisterModel( const char *name ) {
			return UI_IMPORT.R_RegisterModel( name );
		}

		inline struct shader_s *R_RegisterSkin( const char *name ) {
			return UI_IMPORT.R_RegisterSkin( name );
		}

		inline struct shader_s *R_RegisterPic( const char *name ) {
			return UI_IMPORT.R_RegisterPic( name );
		}

		inline struct shader_s *R_RegisterRawPic( const char *name, int width, int height, qbyte *data ) {
			return UI_IMPORT.R_RegisterRawPic( name, width, height, data );
		}

		inline struct shader_s *R_RegisterLevelshot( const char *name, struct shader_s *defaultPic, qboolean *matchesDefault ) {
			return UI_IMPORT.R_RegisterLevelshot( name, defaultPic, matchesDefault );
		}

		inline struct skinfile_s *R_RegisterSkinFile( const char *name ) {
			return UI_IMPORT.R_RegisterSkinFile( name );
		}

		inline struct shader_s *R_RegisterVideo( const char *name ) {
			return UI_IMPORT.R_RegisterVideo( name );
		}

		inline void R_GetShaderDimensions( const struct shader_s *shader, int *width, int *height, int *depth ) {
			UI_IMPORT.R_GetShaderDimensions( shader, width, height, depth );
		}

		inline qboolean R_LerpTag( orientation_t *orient, struct model_s *mod, int oldframe, int frame, float lerpfrac, const char *name ) {
			return UI_IMPORT.R_LerpTag( orient, mod, oldframe, frame, lerpfrac, name );
		}

		inline void R_DrawStretchPoly( const poly_t *poly, float x_offset, float y_offset ) {
			UI_IMPORT.R_DrawStretchPoly( poly, x_offset, y_offset );
		}

		inline void R_DrawStretchPic( int x, int y, int w, int h, float s1, float t1, float s2, float t2, vec4_t color, struct shader_s *shader ) {
			UI_IMPORT.R_DrawStretchPic( x, y, w, h, s1, t1, s2, t2, color, shader );
		}

		inline void R_DrawRotatedStretchPic( int x, int y, int w, int h, float s1, float t1, float s2, float t2, float angle, vec4_t color, struct shader_s *shader ) {
			UI_IMPORT.R_DrawRotatedStretchPic( x, y, w, h, s1, t1, s2, t2, angle, color, shader );
		}

		inline void R_TransformVectorToScreen( refdef_t *rd, vec3_t in, vec2_t out ) {
			UI_IMPORT.R_TransformVectorToScreen( rd, in, out );
		}

		inline int R_SkeletalGetNumBones( struct model_s *mod, int *numFrames ) {
			return UI_IMPORT.R_SkeletalGetNumBones( mod, numFrames );
		}

		inline int R_SkeletalGetBoneInfo( struct model_s *mod, int bone, char *name, int size, int *flags ) {
			return UI_IMPORT.R_SkeletalGetBoneInfo( mod, bone, name, size, flags );
		}

		inline void R_SkeletalGetBonePose( struct model_s *mod, int bone, int frame, bonepose_t *bonepose ) {
			UI_IMPORT.R_SkeletalGetBonePose( mod, bone, frame, bonepose );
		}

		inline const char *ML_GetFullname( const char *name ) {
			return UI_IMPORT.ML_GetFullname( name );
		}
		inline const char *ML_GetFilename( const char *fullname ) {
			return UI_IMPORT.ML_GetFilename( fullname );
		}
		inline size_t ML_GetMapByNum( int num, char *out, size_t size ) {
				return UI_IMPORT.ML_GetMapByNum( num, out, size );
		}

		inline struct sfx_s *S_RegisterSound( const char *name ) {
			return UI_IMPORT.S_RegisterSound( name );
		}

		inline void S_StartLocalSound( const char *s ) {
			UI_IMPORT.S_StartLocalSound( s );
		}

		inline void S_StartBackgroundTrack( const char *intro, const char *loop ) {
			UI_IMPORT.S_StartBackgroundTrack( intro, loop );
		}

		inline void S_StopBackgroundTrack( void ) {
			UI_IMPORT.S_StopBackgroundTrack ();
		}

		inline struct mufont_s *SCR_RegisterFont( const char *name ) {
			return UI_IMPORT.SCR_RegisterFont( name );
		}

		inline void SCR_DrawString( int x, int y, int align, const char *str, struct mufont_s *font, vec4_t color ) {
			UI_IMPORT.SCR_DrawString( x, y, align, str, font, color );
		}

		inline int SCR_DrawStringWidth( int x, int y, int align, const char *str, int maxwidth, struct mufont_s *font, vec4_t color ) {
			return UI_IMPORT.SCR_DrawStringWidth( x, y, align, str, maxwidth, font, color );
		}

		inline void SCR_DrawClampString( int x, int y, const char *str, int xmin, int ymin, int xmax, int ymax, struct mufont_s *font, vec4_t color ) {
			UI_IMPORT.SCR_DrawClampString( x, y, str, xmin, ymin, xmax, ymax, font, color );
		}

		inline size_t SCR_strHeight( struct mufont_s *font ) {
			return UI_IMPORT.SCR_strHeight( font );
		}

		inline size_t SCR_strWidth( const char *str, struct mufont_s *font, int maxlen ) {
			return UI_IMPORT.SCR_strWidth( str, font, maxlen );
		}

		inline size_t SCR_StrlenForWidth( const char *str, struct mufont_s *font, size_t maxwidth ) {
			return UI_IMPORT.SCR_StrlenForWidth( str, font, maxwidth );
		}

		inline void CL_Quit( void ) {
			UI_IMPORT.CL_Quit ();
		}

		inline void CL_SetKeyDest( int key_dest ) {
			UI_IMPORT.CL_SetKeyDest( key_dest );
		}

		inline void CL_ResetServerCount( void ) {
			UI_IMPORT.CL_ResetServerCount ();
		}

		inline char *CL_GetClipboardData( qboolean primary ) {
			return UI_IMPORT.CL_GetClipboardData( primary );
		}

		inline void CL_FreeClipboardData( char *data ) {
			UI_IMPORT.CL_FreeClipboardData( data );
		}

		inline void CL_OpenURLInBrowser( const char *url ) {
			UI_IMPORT.CL_OpenURLInBrowser( url );
		}
			
		inline size_t CL_ReadDemoMetaData( const char *demopath, char *meta_data, size_t meta_data_size ) {
			return UI_IMPORT.CL_ReadDemoMetaData( demopath, meta_data, meta_data_size );
		}

		inline const char *Key_GetBindingBuf( int binding ) {
			return UI_IMPORT.Key_GetBindingBuf( binding );
		}

		inline void Key_ClearStates( void ) {
			UI_IMPORT.Key_ClearStates ();
		}

		inline const char *Key_KeynumToString( int keynum ) {
			return UI_IMPORT.Key_KeynumToString( keynum );
		}

		inline int Key_StringToKeynum( const char *s ) {
			return UI_IMPORT.Key_StringToKeynum( s );
		}

		inline void Key_SetBinding( int keynum, const char *binding ) {
			UI_IMPORT.Key_SetBinding( keynum, binding );
		}

		inline qboolean Key_IsDown( int keynum ) {
			return UI_IMPORT.Key_IsDown( keynum );
		}

		inline qboolean VID_GetModeInfo( int *width, int *height, qboolean *wideScreen, int mode ) {
			return UI_IMPORT.VID_GetModeInfo( width, height, wideScreen, mode );
		}

		inline void VID_FlashWindow( int count ) {
			UI_IMPORT.VID_FlashWindow( count );
		}

		inline void GetConfigString( int i, char *str, int size ) {
			UI_IMPORT.GetConfigString( i, str, size );
		}

		inline unsigned int Milliseconds( void ) {
			return UI_IMPORT.Milliseconds ();
		}

		inline unsigned int Microseconds( void ) {
			return UI_IMPORT.Microseconds ();
		}

		inline unsigned int Hash_BlockChecksum( const qbyte * data, size_t len ) {
			return UI_IMPORT.Hash_BlockChecksum( data, len );
		}

		inline unsigned int Hash_SuperFastHash( const qbyte * data, size_t len, unsigned int seed ) {
			return UI_IMPORT.Hash_SuperFastHash( data, len, seed );
		}

		inline int FS_FOpenFile( const char *filename, int *filenum, int mode ) {
			return UI_IMPORT.FS_FOpenFile( filename, filenum, mode );
		}

		inline int FS_Read( void *buffer, size_t len, int file ) {
			return UI_IMPORT.FS_Read( buffer, len, file );
		}

		inline int FS_Write( const void *buffer, size_t len, int file ) {
			return UI_IMPORT.FS_Write( buffer, len, file );
		}

		inline int FS_Tell( int file ) {
			return UI_IMPORT.FS_Tell( file );
		}

		inline int FS_Seek( int file, int offset, int whence ) {
			return UI_IMPORT.FS_Seek( file, offset, whence );
		}

		inline int FS_Eof( int file ) {
			return UI_IMPORT.FS_Eof( file );
		}

		inline int FS_Flush( int file ) {
			return UI_IMPORT.FS_Flush( file );
		}

		inline void FS_FCloseFile( int file ) {
			UI_IMPORT.FS_FCloseFile( file );
		}

		inline qboolean FS_RemoveFile( const char *filename ) {
			return UI_IMPORT.FS_RemoveFile( filename );
		}

		inline qboolean FS_RemoveDirectory( const char *dirname ) {
			return UI_IMPORT.FS_RemoveDirectory( dirname );
		}

		inline int FS_GetFileList( const char *dir, const char *extension, char *buf, size_t bufsize, int start, int end ) {
			return UI_IMPORT.FS_GetFileList( dir, extension, buf, bufsize, start, end );
		}

		inline  int FS_GetGameDirectoryList( char *buf, size_t bufsize ) {
			return UI_IMPORT.FS_GetGameDirectoryList( buf, bufsize );
		}

		inline const char *FS_FirstExtension( const char *filename, const char **extensions, int num_extensions ) {
			return UI_IMPORT.FS_FirstExtension( filename, extensions, num_extensions );
		}

		inline qboolean FS_MoveFile( const char *src, const char *dst ) {
			return UI_IMPORT.FS_MoveFile( src, dst );
		}

		inline qboolean FS_IsUrl( const char *url ) {
			return UI_IMPORT.FS_IsUrl( url );
		}

		inline time_t FS_FileMTime( const char *filename ) {
			return UI_IMPORT.FS_FileMTime( filename );
		}

		inline cvar_t *Cvar_Get( const char *name, const char *value, int flags ) {
			return UI_IMPORT.Cvar_Get( name, value, flags );
		}

		inline cvar_t *Cvar_Set( const char *name, const char *value ) {
			return UI_IMPORT.Cvar_Set( name, value );
		}

		inline void Cvar_SetValue( const char *name, float value ) {
			UI_IMPORT.Cvar_SetValue( name, value );
		}

		inline cvar_t *Cvar_ForceSet( const char *name, const char *value ) {
			return UI_IMPORT.Cvar_ForceSet( name, value );
		}

		inline float Cvar_Value( const char *name ) {
			return UI_IMPORT.Cvar_Value( name );
		}

		inline int Cvar_Int( const char *name ) {
			return (int) UI_IMPORT.Cvar_Value( name );
		}

		inline const char *Cvar_String( const char *name ) {
			return UI_IMPORT.Cvar_String( name );
		}

		// dynvars
		inline dynvar_t *Dynvar_Create( const char *name, qboolean console, dynvar_getter_f getter, dynvar_setter_f setter ) {
			return UI_IMPORT.Dynvar_Create( name, console, getter, setter );
		}

		inline void Dynvar_Destroy( dynvar_t *dynvar ) {
			UI_IMPORT.Dynvar_Destroy( dynvar );
		}

		inline dynvar_t *Dynvar_Lookup( const char *name ) {
			return UI_IMPORT.Dynvar_Lookup( name );
		}

		inline const char *Dynvar_GetName( dynvar_t *dynvar ) {
			return UI_IMPORT.Dynvar_GetName( dynvar );
		}

		inline dynvar_get_status_t Dynvar_GetValue( dynvar_t *dynvar, void **value ) {
			return UI_IMPORT.Dynvar_GetValue( dynvar, value );
		}

		inline dynvar_set_status_t Dynvar_SetValue( dynvar_t *dynvar, void *value ) {
			return UI_IMPORT.Dynvar_SetValue( dynvar, value );
		}

		inline void Dynvar_AddListener( dynvar_t *dynvar, dynvar_listener_f listener ) {
			UI_IMPORT.Dynvar_AddListener( dynvar, listener );
		}

		inline void Dynvar_RemoveListener( dynvar_t *dynvar, dynvar_listener_f listener ) {
			UI_IMPORT.Dynvar_RemoveListener( dynvar, listener );
		}

		//console args
		inline int Cmd_Argc( void ) {
			return UI_IMPORT.Cmd_Argc ();
		}

		inline char *Cmd_Argv( int arg ) {
			return UI_IMPORT.Cmd_Argv( arg );
		}

		inline char *Cmd_Args( void ) {
			return UI_IMPORT.Cmd_Args ();
		}

		inline void *Mem_Alloc( size_t size, const char *filename, int fileline ) {
			return UI_IMPORT.Mem_Alloc( size, filename, fileline );
		}

		inline void Mem_Free( void *data, const char *filename, int fileline ) {
			UI_IMPORT.Mem_Free( data, filename, fileline );
		}

		inline struct angelwrap_api_s *asGetAngelExport( void ) {
			return UI_IMPORT.asGetAngelExport();
		}

		inline void AsyncStream_UrlEncode( const char *src, char *dst, size_t size ) {
			UI_IMPORT.AsyncStream_UrlEncode( src, dst, size );
		}

		inline size_t AsyncStream_UrlDecode( const char *src, char *dst, size_t size ) {
			return UI_IMPORT.AsyncStream_UrlDecode( src, dst, size );
		}

		inline int AsyncStream_PerformRequest( const char *url, const char *method, const char *data, int timeout,
			ui_async_stream_read_cb_t read_cb, ui_async_stream_done_cb_t done_cb, void *privatep ) {
				return UI_IMPORT.AsyncStream_PerformRequest( url, method, data, timeout, read_cb, done_cb, privatep );
		}

		inline qboolean MM_Login( const char *user, const char *password ) {
			return UI_IMPORT.MM_Login( user, password );
		}

		inline qboolean MM_Logout( qboolean force ) {
			return UI_IMPORT.MM_Logout( force );
		}

		inline int MM_GetLoginState( void ) {
			return UI_IMPORT.MM_GetLoginState();
		}

		inline size_t MM_GetLastErrorMessage( char *buffer, size_t buffer_size ) {
			return UI_IMPORT.MM_GetLastErrorMessage( buffer, buffer_size );
		}

		inline size_t MM_GetProfileURL( char *buffer, size_t buffer_size, qboolean rml ) {
			return UI_IMPORT.MM_GetProfileURL( buffer, buffer_size, rml );
		}

		inline size_t MM_GetBaseWebURL( char *buffer, size_t buffer_size ) {
			return UI_IMPORT.MM_GetBaseWebURL( buffer, buffer_size );
		}

		// IRC
		inline size_t Irc_HistorySize( void ) {
			return UI_IMPORT.Irc_HistorySize();
		}

		inline size_t Irc_HistoryTotalSize(void) {
			return UI_IMPORT.Irc_HistoryTotalSize();
		}

		// history is in reverse order (newest line first)
		inline const struct irc_chat_history_node_s *Irc_GetHistoryHeadNode(void) {
			return UI_IMPORT.Irc_GetHistoryHeadNode();
		}

		inline const struct irc_chat_history_node_s *Irc_GetNextHistoryNode(const struct irc_chat_history_node_s *n) {
			return UI_IMPORT.Irc_GetNextHistoryNode(n);
		}

		inline const struct irc_chat_history_node_s *Irc_GetPrevHistoryNode(const struct irc_chat_history_node_s *n) {
			return UI_IMPORT.Irc_GetPrevHistoryNode(n);
		}

		inline const char *Irc_GetHistoryNodeLine(const struct irc_chat_history_node_s *n) {
			return UI_IMPORT.Irc_GetHistoryNodeLine(n);
		}
}

#endif
