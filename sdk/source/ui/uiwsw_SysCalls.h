/*
Copyright (C) 1997-2001 Id Software, Inc.

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

#ifndef _UIWSW_SYSCALLS_H_
#define _UIWSW_SYSCALLS_H_

#include "ui_public.h"

extern "C" ui_export_t *GetUIAPI( ui_import_t *import );

namespace UIWsw
{
	class Trap
	{
		friend ui_export_t *::GetUIAPI( ui_import_t *import );

	private:
		static ui_import_t UI_IMPORT;

	public:
		static inline void Error( char *str ) {
			UI_IMPORT.Error( str );
		}

		static inline void Print( char *str ) {
			UI_IMPORT.Print( str );
		}

		static inline void Cmd_AddCommand( const char *name, void(*cmd)(void) ) {
			UI_IMPORT.Cmd_AddCommand( name, cmd );
		}

		static inline void Cmd_RemoveCommand( const char *cmd_name ) {
			UI_IMPORT.Cmd_RemoveCommand( cmd_name );
		}

		static inline void Cmd_ExecuteText( int exec_when, const char *text ) {
			UI_IMPORT.Cmd_ExecuteText( exec_when, text );
		}

		static inline void Cmd_Execute( void ) {
			UI_IMPORT.Cmd_Execute ();
		}

		static inline void R_ClearScene( void ) {
			UI_IMPORT.R_ClearScene ();
		}

		static inline void R_AddEntityToScene( entity_t *ent ) {
			UI_IMPORT.R_AddEntityToScene( ent );
		}

		static inline void R_AddLightToScene( vec3_t org, float intensity, float r, float g, float b, struct shader_s *shader ) {
			UI_IMPORT.R_AddLightToScene( org, intensity, r, g, b, shader );
		}

		static inline void R_AddPolyToScene( poly_t *poly ) {
			UI_IMPORT.R_AddPolyToScene( poly );
		}

		static inline void R_RenderScene( refdef_t *fd ) {
			UI_IMPORT.R_RenderScene( fd );
		}

		static inline void R_EndFrame( void ) {
			UI_IMPORT.R_EndFrame ();
		}

		static inline void R_ModelBounds( struct model_s *mod, vec3_t mins, vec3_t maxs ) {
			UI_IMPORT.R_ModelBounds( mod, mins, maxs );
		}

		static inline struct model_s *R_RegisterModel( const char *name ) {
			return UI_IMPORT.R_RegisterModel( name );
		}

		static inline struct shader_s *R_RegisterSkin( const char *name ) {
			return UI_IMPORT.R_RegisterSkin( name );
		}

		static inline struct shader_s *R_RegisterPic( const char *name ) {
			return UI_IMPORT.R_RegisterPic( name );
		}

		static inline struct shader_s *R_RegisterLevelshot( const char *name, struct shader_s *defaultPic ) {
			return UI_IMPORT.R_RegisterLevelshot( name, defaultPic );
		}

		static inline struct skinfile_s *R_RegisterSkinFile( const char *name ) {
			return UI_IMPORT.R_RegisterSkinFile( name );
		}

		static inline qboolean R_LerpTag( orientation_t *orient, struct model_s *mod, int oldframe, int frame, float lerpfrac, const char *name ) {
			return UI_IMPORT.R_LerpTag( orient, mod, oldframe, frame, lerpfrac, name );
		}

		static inline void R_DrawStretchPic( int x, int y, int w, int h, float s1, float t1, float s2, float t2, vec4_t color, struct shader_s *shader ) {
			UI_IMPORT.R_DrawStretchPic( x, y, w, h, s1, t1, s2, t2, color, shader );
		}

		static inline void R_TransformVectorToScreen( refdef_t *rd, vec3_t in, vec2_t out ) {
			UI_IMPORT.R_TransformVectorToScreen( rd, in, out );
		}

		static inline int R_SkeletalGetNumBones( struct model_s *mod, int *numFrames ) {
			return UI_IMPORT.R_SkeletalGetNumBones( mod, numFrames );
		}

		static inline int R_SkeletalGetBoneInfo( struct model_s *mod, int bone, char *name, int size, int *flags ) {
			return UI_IMPORT.R_SkeletalGetBoneInfo( mod, bone, name, size, flags );
		}

		static inline void R_SkeletalGetBonePose( struct model_s *mod, int bone, int frame, bonepose_t *bonepose ) {
			UI_IMPORT.R_SkeletalGetBonePose( mod, bone, frame, bonepose );
		}

//		static inline char *CM_LoadMapMessage( char *name, char *message, int size ) {
//			return UI_IMPORT.CM_LoadMapMessage( name, message, size );
//		}
		static inline const char *ML_GetFullname( const char *name ) {
			return UI_IMPORT.ML_GetFullname( name );
		}
		static inline const char *ML_GetFilename( const char *fullname ) {
			return UI_IMPORT.ML_GetFilename( fullname );
		}
		static inline size_t ML_GetMapByNum( int num, char *out, size_t size ) {
				return UI_IMPORT.ML_GetMapByNum( num, out, size );
		}

		static inline struct sfx_s *S_RegisterSound( const char *name ) {
			return UI_IMPORT.S_RegisterSound( name );
		}

		static inline void S_StartLocalSound( char *s ) {
			UI_IMPORT.S_StartLocalSound( s );
		}

		static inline void S_StartBackgroundTrack( char *intro, char *loop ) {
			UI_IMPORT.S_StartBackgroundTrack( intro, loop );
		}

		static inline void S_StopBackgroundTrack( void ) {
			UI_IMPORT.S_StopBackgroundTrack ();
		}

		static inline struct mufont_s *SCR_RegisterFont( const char *name ) {
			return UI_IMPORT.SCR_RegisterFont( name );
		}

		static inline void SCR_DrawString( int x, int y, int align, const char *str, struct mufont_s *font, vec4_t color ) {
			UI_IMPORT.SCR_DrawString( x, y, align, str, font, color );
		}

		static inline int SCR_DrawStringWidth( int x, int y, int align, const char *str, int maxwidth, struct mufont_s *font, vec4_t color ) {
			return UI_IMPORT.SCR_DrawStringWidth( x, y, align, str, maxwidth, font, color );
		}

		static inline void SCR_DrawClampString( int x, int y, const char *str, int xmin, int ymin, int xmax, int ymax, struct mufont_s *font, vec4_t color ) {
			UI_IMPORT.SCR_DrawClampString( x, y, str, xmin, ymin, xmax, ymax, font, color );
		}

		static inline size_t SCR_strHeight( struct mufont_s *font ) {
			return UI_IMPORT.SCR_strHeight( font );
		}

		static inline size_t SCR_strWidth( const char *str, struct mufont_s *font, int maxlen ) {
			return UI_IMPORT.SCR_strWidth( str, font, maxlen );
		}

		static inline size_t SCR_StrlenForWidth( const char *str, struct mufont_s *font, size_t maxwidth ) {
			return UI_IMPORT.SCR_StrlenForWidth( str, font, maxwidth );
		}

		static inline void CL_Quit( void ) {
			UI_IMPORT.CL_Quit ();
		}

		static inline void CL_SetKeyDest( int key_dest ) {
			UI_IMPORT.CL_SetKeyDest( key_dest );
		}

		static inline void CL_ResetServerCount( void ) {
			UI_IMPORT.CL_ResetServerCount ();
		}

		static inline char *CL_GetClipboardData( qboolean primary ) {
			return UI_IMPORT.CL_GetClipboardData( primary );
		}

		static inline void CL_FreeClipboardData( char *data ) {
			UI_IMPORT.CL_FreeClipboardData( data );
		}

		static inline const char *Key_GetBindingBuf( int binding ) {
			return UI_IMPORT.Key_GetBindingBuf( binding );
		}

		static inline void Key_ClearStates( void ) {
			UI_IMPORT.Key_ClearStates ();
		}

		static inline const char *Key_KeynumToString( int keynum ) {
			return UI_IMPORT.Key_KeynumToString( keynum );
		}

		static inline void Key_SetBinding( int keynum, char *binding ) {
			UI_IMPORT.Key_SetBinding( keynum, binding );
		}

		static inline qboolean Key_IsDown( int keynum ) {
			return UI_IMPORT.Key_IsDown( keynum );
		}

		static inline qboolean VID_GetModeInfo( int *width, int *height, qboolean *wideScreen, int mode ) {
			return UI_IMPORT.VID_GetModeInfo( width, height, wideScreen, mode );
		}

		static inline void GetConfigString( int i, char *str, int size ) {
			UI_IMPORT.GetConfigString( i, str, size );
		}

		static inline unsigned int Milliseconds( void ) {
			return UI_IMPORT.Milliseconds ();
		}

		static inline int FS_FOpenFile( const char *filename, int *filenum, int mode ) {
			return UI_IMPORT.FS_FOpenFile( filename, filenum, mode );
		}

		static inline int FS_Read( void *buffer, size_t len, int file ) {
			return UI_IMPORT.FS_Read( buffer, len, file );
		}

		static inline int FS_Write( const void *buffer, size_t len, int file ) {
			return UI_IMPORT.FS_Write( buffer, len, file );
		}

		static inline int FS_Tell( int file ) {
			return UI_IMPORT.FS_Tell( file );
		}

		static inline int FS_Seek( int file, int offset, int whence ) {
			return UI_IMPORT.FS_Seek( file, offset, whence );
		}

		static inline int FS_Eof( int file ) {
			return UI_IMPORT.FS_Eof( file );
		}

		static inline int FS_Flush( int file ) {
			return UI_IMPORT.FS_Flush( file );
		}

		static inline void FS_FCloseFile( int file ) { 
			UI_IMPORT.FS_FCloseFile( file );
		}

		static inline int FS_GetFileList( const char *dir, const char *extension, char *buf, size_t bufsize, int start, int end ) {
			return UI_IMPORT.FS_GetFileList( dir, extension, buf, bufsize, start, end );
		}

		static inline  int FS_GetGameDirectoryList( char *buf, size_t bufsize ) {
			return UI_IMPORT.FS_GetGameDirectoryList( buf, bufsize );
		}

		static inline const char *FS_FirstExtension( const char *filename, const char **extensions, int num_extensions ) {
			return UI_IMPORT.FS_FirstExtension( filename, extensions, num_extensions );
		}

		static inline cvar_t *Cvar_Get( const char *name, const char *value, int flags ) {
			return UI_IMPORT.Cvar_Get( name, value, flags );
		}

		static inline cvar_t *Cvar_Set( const char *name, const char *value ) {
			return UI_IMPORT.Cvar_Set( name, value );
		}

		static inline void Cvar_SetValue( const char *name, float value ) {
			UI_IMPORT.Cvar_SetValue( name, value );
		}

		static inline cvar_t *Cvar_ForceSet( const char *name, const char *value ) {
			return UI_IMPORT.Cvar_ForceSet( name, value );
		}

		static inline float Cvar_Value( const char *name ) {
			return UI_IMPORT.Cvar_Value( name );
		}

		static inline int Cvar_Int( const char *name ) {
			return (int) UI_IMPORT.Cvar_Value( name );
		}

		static inline const char *Cvar_String( const char *name ) {
			return UI_IMPORT.Cvar_String( name );
		}

		// dynvars
		static inline dynvar_t *Dynvar_Create( const char *name, qboolean console, dynvar_getter_f getter, dynvar_setter_f setter ) {
			return UI_IMPORT.Dynvar_Create( name, console, getter, setter );
		}

		static inline void Dynvar_Destroy( dynvar_t *dynvar ) {
			UI_IMPORT.Dynvar_Destroy( dynvar );
		}

		static inline dynvar_t *Dynvar_Lookup( const char *name ) {
			return UI_IMPORT.Dynvar_Lookup( name );
		}

		static inline const char *Dynvar_GetName( dynvar_t *dynvar ) {
			return UI_IMPORT.Dynvar_GetName( dynvar );
		}

		static inline dynvar_get_status_t Dynvar_GetValue( dynvar_t *dynvar, void **value ) {
			return UI_IMPORT.Dynvar_GetValue( dynvar, value );
		}

		static inline dynvar_set_status_t Dynvar_SetValue( dynvar_t *dynvar, void *value ) {
			return UI_IMPORT.Dynvar_SetValue( dynvar, value );
		}

		static inline void Dynvar_AddListener( dynvar_t *dynvar, dynvar_listener_f listener ) {
			UI_IMPORT.Dynvar_AddListener( dynvar, listener );
		}

		static inline void Dynvar_RemoveListener( dynvar_t *dynvar, dynvar_listener_f listener ) {
			UI_IMPORT.Dynvar_RemoveListener( dynvar, listener );
		}

		//console args
		static inline int Cmd_Argc( void ) {
			return UI_IMPORT.Cmd_Argc ();
		}

		static inline char *Cmd_Argv( int arg ) {
			return UI_IMPORT.Cmd_Argv( arg );
		}

		static inline char *Cmd_Args( void ) {
			return UI_IMPORT.Cmd_Args ();
		}

		static inline void *Mem_Alloc( size_t size, const char *filename, int fileline ) {
			return UI_IMPORT.Mem_Alloc( size, filename, fileline );
		}

		static inline void Mem_Free( void *data, const char *filename, int fileline ) {
			UI_IMPORT.Mem_Free( data, filename, fileline );
		}

		static inline void MM_UIRequest( mm_action_t action, const char *data )
		{
			UI_IMPORT.MM_UIRequest( action, data );
		}

		static inline mm_status_t MM_GetStatus( void )
		{
			return UI_IMPORT.MM_GetStatus();
		}
	};
}

#endif
