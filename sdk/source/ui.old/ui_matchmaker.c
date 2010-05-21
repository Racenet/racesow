/*

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

#include "ui_local.h"

#if 0

#define MM_COLUMN_OFFSET 100

#define CHATBOX_HEIGHT 9
#define CHATBOX_HALFWIDTH 250
m_itemslisthead_t chatmsgs;
static int chatpos;

// callback funcs
#ifdef AUTH_CODE
static void MatchMaker_AuthReply_Callback( auth_reply_t reply );
static void MatchMaker_Login_Callback( qboolean status );
#endif

// control handlers
static void MatchMaker_GameTypeControl( menucommon_t *menuitem );
static void MatchMaker_StartButton( menucommon_t *menuitem );
#ifdef AUTH_CODE
static void MatchMaker_LoginButton( menucommon_t *unused );
static void MatchMaker_SkillControl( menucommon_t *menuitem );
#endif

#define mm_started trap_MM_MatchStatus( NULL )

// local utility funcs
static void MatchMaker_UpdateButtons( void );

static menuframework_s s_matchmaker_menu;

// current ping/gametype/skill spincontrol value
static int cur_ping;
static int cur_gametype;
static int cur_skill;

// spincontrol arrays
static char **gametypes = NULL;
static char *anydependent[] = { "any", "dependent", 0 };

static int buttonyoffset;

//================
// CALLBACK FUNCS
//================

//================
// MatchMaker_Popped
// Called when the matchmaker menu is popped
//================
static void MatchMaker_Popped( void )
{
	m_listitem_t *item, *next;

	if( mm_started )
		trap_MM_Drop();

	// clear chatbox
	for( item = chatmsgs.headNode ; item ; item = next )
	{
		UI_Free( item->data );
		next = item->pnext;
		UI_Free( item );
	}

	M_MatchMaker_UpdateStatus( "", qfalse );

	memset( &chatmsgs, 0, sizeof( chatmsgs ) );
}

//================
// MatchMaker_AuthReply_Callback
// This will tell us the result of the auth request
//================
#ifdef AUTH_CODE
static void MatchMaker_AuthReply_Callback( auth_reply_t reply )
{
	mm_loggedin = reply == AUTH_VALID ? qtrue : qfalse;
	MatchMaker_UpdateButtons();
}

//================
// MatchMaker_Login_Callback
// This will tell us the result of the login popup
//================
static void MatchMaker_Login_Callback( qboolean status )
{
	// store whether or not we are logged in
	mm_loggedin = status;
	MatchMaker_UpdateButtons();
}
#endif

//================
// CONTROL HANDLERS
//================
static void MatchMaker_PingControl( menucommon_t *menuitem )
{
	if( mm_started )
	{
		// dont let people who have started matchmaking change the gametype
		menuitem->curvalue = cur_ping;
		M_MatchMaker_UpdateStatus( "you cannot change gametype once you have started matchmaking", qfalse );
		return;
	}

	cur_ping = menuitem->curvalue;
}

static void MatchMaker_GameTypeControl( menucommon_t *menuitem )
{
	int i, maxclients;

	if( mm_started )
	{
		// dont let people who have started matchmaking change the gametype
		menuitem->curvalue = cur_gametype;
		M_MatchMaker_UpdateStatus( "you cannot change gametype once you have started matchmaking", qfalse );
		return;
	}

	cur_gametype = menuitem->curvalue;

	MM_GetGameTypeInfo( MM_GetGameTypeTagByName( gametypes[cur_gametype] ), &maxclients, NULL, NULL, NULL );

	for( i = 0 ; i < 8 ; i++ )
	{
		if( i < maxclients )
			M_MatchMaker_UpdateSlot( i, va( "Slot %d", i + 1 ) );
		else
			M_MatchMaker_UpdateSlot( i, "" );
	}
}

#ifdef AUTH_CODE
static void MatchMaker_SkillControl( menucommon_t *menuitem )
{
	if( mm_started )
	{
		menuitem->curvalue = cur_skill;
		M_MatchMaker_UpdateStatus( "you cannot change skill range once you have started matchmaking", qfalse );
		return;
	}

	if( !mm_loggedin )
	{
		// unregistered users cannot play skil dependent matches
		menuitem->curvalue = cur_skill;
		M_MatchMaker_UpdateStatus( "you must be logged in to play skill dependent matches", qtrue );
		return;
	}

	cur_skill = menuitem->curvalue;
}

static void MatchMaker_LoginButton( menucommon_t *unused )
{
	M_Login_Callback = MatchMaker_Login_Callback;
	trap_Cmd_ExecuteText( EXEC_APPEND, "menu_login" );
}
#endif

static void MatchMaker_StartButton( menucommon_t *menuitem )
{
#ifndef ALLOW_UNREGISTERED_MM
	if( !mm_loggedin )
	{
		M_MatchMaker_UpdateStatus( "you must be logged in to start matchmaking", qtrue );
		return;
	}
#endif

	if( mm_started )
	{
		trap_MM_Drop();
		M_MatchMaker_UpdateStatus( "press start to begin matchmaking", qtrue );
	}
	else
		trap_MM_Join( cur_skill, MM_GetGameTypeTagByName( gametypes[cur_gametype] ) );

	MatchMaker_UpdateButtons();
}

static void MatchMaker_UpdateChatScrollBar( menucommon_t *menuitem )
{
	chatpos = menuitem->curvalue;
	trap_Cvar_Set( "m_matchmaker_chat_curvalue", va( "%d", chatpos ) );
}

// This code below is shamefully messy
static void MatchMaker_UpdateChatMsg( menucommon_t *menuitem )
{
	m_listitem_t *item;
	char *data, *nick, *msg;
	int nickwidth, pos, color = -1;
	int *time, *currentpos;

	menuitem->localdata[1] = menuitem->localdata[0] + chatpos;
	item = UI_FindItemInScrollListWithId( &chatmsgs, menuitem->localdata[1] );
	if( !item )
	{
		Q_snprintfz( menuitem->title, MAX_STRING_CHARS, "" );
		return;
	}

	data = UI_Malloc( strlen( item->data ) + 1 );
	strcpy( data, item->data );

	nick = data;
	// line break is added after nickname so we know where to seperate them
	msg = strchr( data, '\n' );
	if( msg )
		*msg = ' ';
	// there is no return character, this is a message added by the menu itself
	// split it at the first space (after the timestamp)
	else
		msg = strchr( data, ' ' );

	// invalid message
	if( !msg )
		return;

	// text is short enough to fit inside the chatbox
	if( 2 * CHATBOX_HALFWIDTH > trap_SCR_strWidth( data, uis.fontSystemSmall, 0 ) )
	{
		Q_snprintfz( menuitem->title, sizeof( menuitem->title ), data );

		UI_Free( data );
		return;
	}

	// split up the nickname and message
	*msg++ = 0;

	// add a space to nickname width
	nickwidth = trap_SCR_strWidth( va( "%s ", nick ), uis.fontSystemSmall, 0 );

	time = &menuitem->localdata[2];
	currentpos = &menuitem->localdata[3];

	// scroll through the message
	*time -= uis.frameTime * 1000;
	if( *time < 0 )
	{
		*time = 170;
		pos = ++(*currentpos);

		// wait longer for the first character
		if( !pos )
			*time += 1000;
	
		// go to the position in the message, searching for colour characters as well
		while( pos )
		{
			if( *msg == Q_COLOR_ESCAPE )
			{
				// store this color, we will use it later
				color = atoi( msg + 1 );
				msg += 2;
				continue;
			}
			msg++;
			pos--;
		}

		// the rest of the message fits in the window
		// reset position
		if( trap_SCR_strWidth( msg, uis.fontSystemSmall, 0 ) + nickwidth < CHATBOX_HALFWIDTH * 2 )
		{
			*currentpos = -1;
			// wait longer at the end
			*time += 500;
		}

		Q_snprintfz( menuitem->title, MAX_STRING_CHARS, "%s %s%s", nick, color > -1 ? va( "%s%d", S_COLOR_ESCAPE, color ) : "", msg );
	} // if *time < 0

	UI_Free( data );
}

static void MatchMaker_SendChatMsg( menucommon_t *unused )
{
	menucommon_t *s;
	char *msg;
	int type;
	menufield_t *field;

	if( !mm_started )
		return;

	s = UI_MenuItemByName( "m_matchmaker_chat_msg" );
	field = (menufield_t *)s->itemlocal;
	msg = UI_GetMenuitemFieldBuffer( s );

	if( !msg || !*msg )
		return;

	s = UI_MenuItemByName( "m_matchmaker_chat_type" );
	type = s->curvalue;

	trap_MM_Chat( type, msg );

	// clear textbox
	field->cursor = 0;
	field->buffer[0] = 0;
}

//================
// MatchMaker_MenuInit
//================
static void MatchMaker_MenuInit( void )
{
	menucommon_t *menuitem;
	int yoffset = 0, tempyoffset = 0, i = 0, scrollid;
	mm_supported_gametypes_t *gametype;
	static char *chat_types[] = { "all", "match", 0 };

	s_matchmaker_menu.nitems = 0;

	if( !gametypes )
	{
		gametypes = (char **)UI_Malloc( ( GAMETYPE_TOTAL + 1 ) * sizeof( char * ) );

		// setup spincontrol data
		for( gametype = supported_gametypes; gametype->short_name; gametype++, i++ )
		{
			gametypes[i] = (char *)UI_Malloc( sizeof( gametype->short_name ) + 1 + 2 ); // +2 for color escape character eg "^7"
			Q_strncpyz( gametypes[i], gametype->short_name, sizeof( gametype->short_name ) + 1 );
		}
		gametypes[i+1] = NULL;
	}

	menuitem = UI_InitMenuItem( "m_matchmaker_title1", "MATCH MAKER", 0, yoffset, MTYPE_SEPARATOR, ALIGN_CENTER_TOP, uis.fontSystemBig, NULL );
	Menu_AddItem( &s_matchmaker_menu, menuitem );
	yoffset += trap_SCR_strHeight( menuitem->font );
	yoffset += trap_SCR_strHeight( menuitem->font );

	tempyoffset = yoffset;

	menuitem = UI_InitMenuItem( "m_matchmaker_ping", "ping range", -MM_COLUMN_OFFSET, tempyoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, MatchMaker_PingControl );
	UI_SetupSpinControl( menuitem, anydependent, 0 );
	Menu_AddItem( &s_matchmaker_menu, menuitem );
	tempyoffset += trap_SCR_strHeight( menuitem->font );

	menuitem = UI_InitMenuItem( "m_matchmaker_gametype", "gametype", -MM_COLUMN_OFFSET, tempyoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, MatchMaker_GameTypeControl );
	UI_SetupSpinControl( menuitem, gametypes, 0 );
	Menu_AddItem( &s_matchmaker_menu, menuitem );
	tempyoffset += trap_SCR_strHeight( menuitem->font );

#ifdef AUTH_CODE
	menuitem = UI_InitMenuItem( "m_matchmaker_skill", "skill range", -MM_COLUMN_OFFSET, tempyoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, MatchMaker_SkillControl );
	UI_SetupSpinControl( menuitem, anydependent, 0 );
	Menu_AddItem( &s_matchmaker_menu, menuitem );
	tempyoffset += trap_SCR_strHeight( menuitem->font );
#endif

	menuitem = UI_InitMenuItem( "m_matchmaker_title2", "players", MM_COLUMN_OFFSET - 30, yoffset, MTYPE_SEPARATOR, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_matchmaker_menu, menuitem );

	for( i = 0; i < 8; i++ )
	{
		menuitem = UI_InitMenuItem( va( "m_matchmaker_slot%d", i ), va( "%sSlot %d", S_COLOR_WHITE, i + 1 ), MM_COLUMN_OFFSET, yoffset, MTYPE_SEPARATOR, ALIGN_LEFT_TOP, uis.fontSystemSmall, NULL );
		Menu_AddItem( &s_matchmaker_menu, menuitem );
		yoffset += trap_SCR_strHeight( menuitem->font );
	}

	buttonyoffset = yoffset;
	MatchMaker_UpdateButtons();
	yoffset += trap_SCR_strHeight( uis.fontSystemBig );
	yoffset += trap_SCR_strHeight( uis.fontSystemBig );

	menuitem = UI_InitMenuItem( "m_matchmaker_chat", NULL, CHATBOX_HALFWIDTH, yoffset, MTYPE_SCROLLBAR, ALIGN_RIGHT_TOP, uis.fontSystemSmall, MatchMaker_UpdateChatScrollBar );
	menuitem->scrollbar_id = scrollid = s_matchmaker_menu.nitems;
	Menu_AddItem( &s_matchmaker_menu, menuitem );
	Q_strncpyz( menuitem->title, "m_matchmaker_chat_curvalue", sizeof( menuitem->title ) );
	UI_SetupScrollbar( menuitem, CHATBOX_HEIGHT, 0, 0, 0 );
	
	for( i = 0 ; i < CHATBOX_HEIGHT ; i++ )
	{
		menuitem = UI_InitMenuItem( va( "m_matchmaker_chat_%d", i ), "", -CHATBOX_HALFWIDTH, yoffset, MTYPE_SEPARATOR, ALIGN_LEFT_TOP, uis.fontSystemSmall, NULL );
		menuitem->scrollbar_id = scrollid;
		menuitem->ownerdraw = MatchMaker_UpdateChatMsg;
		menuitem->localdata[0] = i;
		menuitem->localdata[1] = i;
		menuitem->localdata[2] = 0; // time until text scrolls again
		menuitem->localdata[3] = -1; // the position it has scrolled to in the message
		menuitem->width = 2 * CHATBOX_HALFWIDTH;

		menuitem->pict.shader = uis.whiteShader;
		
		Vector4Copy( ( i & 1 ) ? colorDkGrey : colorMdGrey, menuitem->pict.color );
		menuitem->pict.color[3] = menuitem->pict.colorHigh[3] = 0.65f;
		menuitem->pict.yoffset = 0;
		menuitem->pict.xoffset = 0;
		menuitem->pict.width = 2 * CHATBOX_HALFWIDTH;
		menuitem->pict.height = trap_SCR_strHeight( menuitem->font );

		Menu_AddItem( &s_matchmaker_menu, menuitem );

		yoffset += trap_SCR_strHeight( menuitem->font );
	}
	yoffset += 5;

	// all initially disabled because users do not start in a match
	menuitem = UI_InitMenuItem( "m_matchmaker_chat_msg", "chat", -CHATBOX_HALFWIDTH - 16, yoffset, MTYPE_FIELD, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_matchmaker_menu, menuitem );
	UI_SetupField( menuitem, "", 100, 2 * CHATBOX_HALFWIDTH - 150 );
	menuitem->disabled = qtrue;

	menuitem = UI_InitMenuItem( "m_matchmaker_chat_type", "send to", CHATBOX_HALFWIDTH - 57, yoffset, MTYPE_SPINCONTROL, ALIGN_RIGHT_TOP, uis.fontSystemSmall, NULL );
	Menu_AddItem( &s_matchmaker_menu, menuitem );
	UI_SetupSpinControl( menuitem, chat_types, 0 );
	menuitem->disabled = qtrue;

	yoffset += trap_SCR_strHeight( menuitem->font );
	
	menuitem = UI_InitMenuItem( "m_matchmaker_chat_send", "send", -CHATBOX_HALFWIDTH, yoffset, MTYPE_ACTION, ALIGN_LEFT_TOP, uis.fontSystemSmall, MatchMaker_SendChatMsg );
	Menu_AddItem( &s_matchmaker_menu, menuitem );
	menuitem->disabled = qtrue;

	Menu_Center( &s_matchmaker_menu );
	Menu_Init( &s_matchmaker_menu, qfalse );

	M_MatchMaker_UpdateStatus( "press start to begin matchmaking", qtrue );
}

//================
// MatchMaker_MenuDraw
// ================
static void MatchMaker_MenuDraw( void )
{
	Menu_AdjustCursor( &s_matchmaker_menu, 1 );
	Menu_Draw( &s_matchmaker_menu );
}

//================
// MatchMaker_MenuKey
//================
static const char *MatchMaker_MenuKey( int key )
{
	// send chat msg when user presses enter in chat box
	// better than using callback func because this doesnt include clicks
	if( key == 13 && Menu_ItemAtCursor( &s_matchmaker_menu ) == UI_MenuItemByName( "m_matchmaker_chat_msg" ) )
		MatchMaker_SendChatMsg( NULL );

	return Default_MenuKey( &s_matchmaker_menu, key );
}

//================
// MatchMaker_MenuCharEvent
//================
static const char *MatchMaker_MenuCharEvent( qwchar key )
{
	return Default_MenuCharEvent( &s_matchmaker_menu, key );
}

//================
// M_Menu_MatchMaker_f
//================
void M_Menu_MatchMaker_f( void )
{
	MatchMaker_MenuInit();

#ifdef AUTH_CODE
	UI_AuthReply_Callback = MatchMaker_AuthReply_Callback;
	trap_Auth_CheckUser( NULL, NULL );
#endif

	M_PushMenu( &s_matchmaker_menu, MatchMaker_MenuDraw, MatchMaker_MenuKey, MatchMaker_MenuCharEvent );
	M_SetupPoppedCallback( MatchMaker_Popped );
}

//=========================================================
//                  utility functions
//=========================================================

//================
// M_MatchMaker_UpdateSlot
// Update a particular slot with a playername
//================
void M_MatchMaker_UpdateSlot( int slotno, const char *playername )
{
	menucommon_t *menuitem = UI_MenuItemByName( va( "m_matchmaker_slot%d", slotno ) );
	if( !menuitem )
		return;

	Q_snprintfz( menuitem->title, sizeof( menuitem->title ), "%s%s", S_COLOR_WHITE, playername );
}

//================
// M_MatchMaker_UpdateSlot
// Update the status message
//================
void M_MatchMaker_UpdateStatus( const char *status, qboolean showchat )
{
	static char *mmstatus = NULL;

	if( mmstatus )
	{
		if( !strcmp( mmstatus, status ) )
			return;
		UI_Free( mmstatus );
	}

	mmstatus = UI_Malloc( strlen( status ) + 1 );
	Q_strncpyz( mmstatus, status, strlen( status ) + 1 );
	Menu_SetStatusBar( &s_matchmaker_menu, mmstatus );

	// add status message to chat box (its more visible)
	if( showchat )
		M_MatchMaker_AddChatMsg( va( "%s%s", S_COLOR_WHITE, status ) );
}

void M_MatchMaker_AddChatMsg( const char *msg )
{
	menucommon_t *s;

	int size = strlen( msg ) + 8 + 1;
	char *msg2 = UI_Malloc( size );

	time_t timestamp = time( NULL );
	struct tm *timestampptr = gmtime( &timestamp );

	strftime( msg2, size, "[%H:%M] ", timestampptr );
	Q_strncatz( msg2, msg, size ); 
	UI_AddItemToScrollList( &chatmsgs, va( "m_matchmaker_chat_msg_%d", chatmsgs.numItems ), ( void * )msg2 );
	s = UI_MenuItemByName( "m_matchmaker_chat" );
	s->maxvalue = max( 0, chatmsgs.numItems - CHATBOX_HEIGHT );
	trap_Cvar_Set( "m_matchmaker_chat_curvalue", va( "%d", s->maxvalue ) );
}

void M_MatchMaker_Update( void )
{
	qboolean started = mm_started;

	MatchMaker_UpdateButtons();
	UI_MenuItemByName( "m_matchmaker_chat_send" )->disabled = !started;
	UI_MenuItemByName( "m_matchmaker_chat_type" )->disabled = !started;
	UI_MenuItemByName( "m_matchmaker_chat_msg" )->disabled = !started;
}

//================
// MatchMaker_UpdateButtons
// Update which buttons should be showing and what they should say
//================
typedef struct
{
	char *name;
	void ( *callback )( struct menucommon_s * );
} button_t;
static button_t buttons[] =
{
	{ "back", M_genericBackFunc },
	{ "start", MatchMaker_StartButton },
#ifdef AUTH_CODE
	{ "login", MatchMaker_LoginButton },
#endif
	{ NULL, NULL }
};
#define MM_BUTTON_SPACING 20
static void MatchMaker_UpdateButtons( void )
{
	menucommon_t *menuitem;
	button_t *button;
	int width = 0, x;

	for( button = buttons; button->name; button++ )
	{
		menuitem = UI_MenuItemByName( va( "m_matchmaker_%s", button->name ) );
		// create the button if it doesnt exist
		if( !menuitem )
			menuitem = UI_InitMenuItem( va( "m_matchmaker_%s", button->name ), button->name, 0, 0, MTYPE_ACTION, ALIGN_LEFT_TOP, uis.fontSystemBig, button->callback );

		// special case adjustments
		//if( !strcmp( button->name, "login" ) )
			//Q_strncpyz( menuitem->title, ( mm_loggedin ? "" : "login" ), sizeof( menuitem->title ) );
		if( !strcmp( button->name, "start" ) )
			Q_strncpyz( menuitem->title, ( mm_started ? "stop" : "start" ), sizeof( menuitem->title ) );

		if( *( menuitem->title ) )
		{
			width += trap_SCR_strWidth( menuitem->title, menuitem->font, sizeof( menuitem->title ) );
			width += MM_BUTTON_SPACING;
		}
	}
	// remove the trailing space after the last button
	width -= MM_BUTTON_SPACING;
	x = width / 2 * -1;

	for( button = buttons; button->name; button++ )
	{
		menuitem = UI_MenuItemByName( va( "m_matchmaker_%s", button->name ) );
		if( !*( menuitem->title ) )
			continue;

		menuitem->x = x;
		menuitem->y = buttonyoffset;
		Menu_AddItem( &s_matchmaker_menu, menuitem );

		x += trap_SCR_strWidth( menuitem->title, menuitem->font, sizeof( menuitem->title ) );
		x += MM_BUTTON_SPACING;
	}
}

#else
void M_Menu_MatchMaker_f( void )
{
}
#endif

