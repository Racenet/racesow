/*
Copyright (C) 2007 Benjamin Litzelmann

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


#include "uiwsw_Utils.h"
#include "uicore_Global.h"
#include "uimenu_JoinServerMenu.h"

#define SERVER_PINGING_MAXRETRYTIMEOUTS 1

#define PING_COL_WIDTH			60
#define PLAYER_COL_WIDTH		100
#define GAMETYPE_COL_WIDTH		80
#define MAP_COL_WIDTH			150
#define NAME_COL_WIDTH			330

#define SRVLIST_WIDTH			( PING_COL_WIDTH + PLAYER_COL_WIDTH + GAMETYPE_COL_WIDTH + MAP_COL_WIDTH + NAME_COL_WIDTH )
#define SRVLIST_HEIGHT			240
#define UI_LINE_HEIGHT			20
#define UI_GAP_WIDTH			10
#define UI_GAP_HEIGHT			5
#define FILTER_BUTTON_WIDTH		140
#define FILTER_GAMETYPES_WIDTH	140
#define LABEL_HEIGHT			20

using namespace UICore;
using namespace UIMenu;
using namespace UIWsw;

JoinServerMenu *joinserver = NULL;

void UIMenu::M_Menu_JoinServer_f( void )
{
	if ( !joinserver )
		joinserver = new JoinServerMenu();

	joinserver->Show();
}

void JoinServerMenu::filterHandler( BaseObject *target )
{
	if ( target == joinserver->showfull )
		Trap::Cvar_SetValue( "ui_filter_full", int(Trap::Cvar_Value("ui_filter_full")+1) % 3 );
	else if ( target == joinserver->showempty )
		Trap::Cvar_SetValue( "ui_filter_empty", int(Trap::Cvar_Value("ui_filter_empty")+1) % 3 );
	else if ( target == joinserver->showpassworded )
		Trap::Cvar_SetValue( "ui_filter_password", int(Trap::Cvar_Value("ui_filter_password")+1) % 3 );
	else if ( target == joinserver->showinstagib )
		Trap::Cvar_SetValue( "ui_filter_instagib", int(Trap::Cvar_Value("ui_filter_instagib")+1) % 3 );
	else if ( target == joinserver->skill )
		Trap::Cvar_SetValue( "ui_filter_skill", int(Trap::Cvar_Value("ui_filter_skill")+1) % 4 );

	joinserver->LoadUserSettings();
}

void JoinServerMenu::gametypeFilterHandler( ListItem *, int position, bool isSelected )
{
	if ( !isSelected )
		return;

	Trap::Cvar_SetValue( "ui_filter_gametype", position );
	joinserver->LoadUserSettings();
}

void JoinServerMenu::textFilterHandler( BaseObject *target, const char *text )
{
    if ( target == joinserver->maxping )
	    Trap::Cvar_SetValue( "ui_filter_ping", atoi( text ) );
    else if ( target == joinserver->matchedname )
	    Trap::Cvar_Set( "ui_filter_name", text );
    joinserver->LoadUserSettings();
}

void JoinServerMenu::refreshHandler( BaseObject *target )
{
	ListItem *item = NULL;
	ServerInfo *si;

    // Clear any previous results/pings/...
	joinserver->refreshingList = false;
	joinserver->pingSortMap.clear();
	joinserver->playerSortMap.clear();
	joinserver->mapSortMap.clear();
	joinserver->gametypeSortMap.clear();
	joinserver->nameSortMap.clear();
	
	joinserver->pingbatch++;

	if ((item = joinserver->serverlist->getSelectedItem(0)) != NULL) {
		// If a previous server was selected, remember :)
		si = (ServerInfo*)(item->getUserData());
		Q_strncpyz(joinserver->last_sel, si->address , sizeof(joinserver->last_sel));
	}
	
	/*
	for ( std::list<ServerInfo*>::iterator it = joinserver->servers.begin() ; it != joinserver->servers.end() ; ++it ) {
		delete(*it);
	}
	joinserver->servers.clear();
	*/

	joinserver->SearchGames( "global" );
    joinserver->refreshingList = true;
	joinserver->serverlist->setFocus();
}

void JoinServerMenu::connectHandler( BaseObject * )
{
	ListItem *item = joinserver->serverlist->getSelectedItem(0);
	ServerInfo *srv;
	if (!item)
		return;
	if ( (srv = (ServerInfo*)item->getUserData()) != NULL ) {
		Trap::Cmd_ExecuteText( EXEC_APPEND, va("connect %s", srv->address) );
	}
}

void JoinServerMenu::backHandler( BaseObject* )
{
	Trap::Cmd_ExecuteText( EXEC_APPEND, "menu_main" );
}

void JoinServerMenu::pingServers( BaseObject *, unsigned int deltatime, const Rect *, const Rect *, bool )
{
    if ( !joinserver->refreshingList )
	    return;

    // TODO : ping firstly servers that match filters
    if ( !joinserver->pingingServer || joinserver->nextServerTime - Local::getTime() > SERVER_PINGING_TIMEOUT )
    {
	    for ( std::list<ServerInfo*>::iterator it = joinserver->servers.begin() ; it != joinserver->servers.end() ; ++it )
	    {
		    if ( (*it)->ping > 999 && (*it)->ping_retries < SERVER_PINGING_MAXRETRYTIMEOUTS )
		    {
			    joinserver->pingingServer = *it;
			    (*it)->ping_retries++;
			    joinserver->nextServerTime = Local::getTime() + SERVER_PINGING_TIMEOUT;
			    Trap::Cmd_ExecuteText( EXEC_APPEND, va( "pingserver %s\n", (*it)->address ) );
			    break;
		    }
	    }
    }
}

void JoinServerMenu::sortHandler( BaseObject *target, bool )
{
    sorttype_t newtype = SORT_PING;
    SwitchButton* button = static_cast<SwitchButton*>(target);
    if ( button == joinserver->sortplayer )
	    newtype = SORT_PLAYER;
    else if ( button == joinserver->sortmap )
	    newtype = SORT_MAP;
    else if ( button == joinserver->sortgametype )
	    newtype = SORT_GAMETYPE;
    else if ( button == joinserver->sortname )
	    newtype = SORT_NAME;

    if ( joinserver->sorttype == newtype )
	    joinserver->reverse_sort = !joinserver->reverse_sort;
    else
    {
	    joinserver->sorttype = newtype;
	    joinserver->reverse_sort = false;
    }
    joinserver->RefreshServerList();
    joinserver->sortping->setPressed( false );
    joinserver->sortplayer->setPressed( false );
    joinserver->sortmap->setPressed( false );
    joinserver->sortgametype->setPressed( false );
    joinserver->sortname->setPressed( false );
    if ( joinserver->reverse_sort )
	    joinserver->arrow->setDisabledImage( Trap::R_RegisterPic( "gfx/ui/arrow_up" ) );
    else
	    joinserver->arrow->setDisabledImage( Trap::R_RegisterPic( "gfx/ui/arrow_down" ) );
    button->setPressed( true );
    joinserver->arrow->setParent( button );
    joinserver->arrow->setPosition( button->getWidth() - 10, 5 );
	joinserver->arrow->setVisible(true);
}

ALLOCATOR_DEFINITION(JoinServerMenu)
DELETER_DEFINITION(JoinServerMenu)

JoinServerMenu::JoinServerMenu()
{
    int yoffset = 0, y2offset = 0, xoffset;
	float x,y, h, w;
	// Initialize some generic variables first so we can use them already
	pingingServer = NULL;
    nextServerTime = 0;
    // Default sort by ping
	sorttype = SORT_PING;
	reverse_sort = false;

	last_sel[0] = '\0';
	pingbatch = 0;

	// Build the UI
    panel = Factory::newPanel( rootPanel, 20, 20, 760, 560 );
    panel->setAfterDrawHandler( pingServers );

    lservers = Factory::newLabel( panel, UI_LINE_HEIGHT, yoffset += UI_GAP_HEIGHT, 150, UI_LINE_HEIGHT, "Server list:" );
	
	// Server list column headers
	xoffset = (int)((panel->getWidth() - SRVLIST_WIDTH) / 2);
	yoffset += 40;
    sortping = Factory::newSwitchButton( panel, xoffset, yoffset, PING_COL_WIDTH, UI_LINE_HEIGHT, "Ping" );
    sortplayer = Factory::newSwitchButton( panel, xoffset += PING_COL_WIDTH, yoffset, PLAYER_COL_WIDTH, UI_LINE_HEIGHT, "Players" );
    sortgametype = Factory::newSwitchButton( panel, xoffset += PLAYER_COL_WIDTH, yoffset, GAMETYPE_COL_WIDTH, UI_LINE_HEIGHT, "Gametype" );
    sortmap = Factory::newSwitchButton( panel, xoffset += GAMETYPE_COL_WIDTH, yoffset, MAP_COL_WIDTH, UI_LINE_HEIGHT, "Map" );
    sortname = Factory::newSwitchButton( panel, xoffset += MAP_COL_WIDTH, yoffset, NAME_COL_WIDTH, UI_LINE_HEIGHT, "Name" );
    StyleSorter( sortping );
    StyleSorter( sortplayer );
    StyleSorter( sortgametype );
    StyleSorter( sortmap );
    StyleSorter( sortname );
    arrow = Factory::newPanel( rootPanel, 0, 0, 10, 10 );
	StyleArrow(arrow, sortping, reverse_sort);
	
	// Server list itself
    serverlist = Factory::newListBox( panel, ((panel->getWidth() - SRVLIST_WIDTH) / 2), yoffset += UI_LINE_HEIGHT, SRVLIST_WIDTH, SRVLIST_HEIGHT, false );
	serverlist->setMultipleSelection(false);
	serverlist->setItemsSwitchable(false);
	serverlist->setItemSelectedHandler(serverItemSelected);


	yoffset += SRVLIST_HEIGHT + UI_GAP_HEIGHT;
	// Filter menu
	filter = Factory::newPanel( panel, UI_GAP_WIDTH, yoffset, 440, 200 );
	y2offset = UI_GAP_HEIGHT;
    lfilter = Factory::newLabel( filter, UI_GAP_WIDTH, y2offset, 100, UI_LINE_HEIGHT, "Filters:" );
    showfull = Factory::newButton( filter, UI_GAP_WIDTH, (y2offset += UI_GAP_HEIGHT + LABEL_HEIGHT), FILTER_BUTTON_WIDTH, UI_LINE_HEIGHT, "Show full" );
    showempty = Factory::newButton( filter, UI_GAP_WIDTH, (y2offset += UI_GAP_HEIGHT + LABEL_HEIGHT), FILTER_BUTTON_WIDTH, UI_LINE_HEIGHT, "Show empty" );
    showpassworded = Factory::newButton( filter, UI_GAP_WIDTH, (y2offset += UI_GAP_HEIGHT + LABEL_HEIGHT), FILTER_BUTTON_WIDTH, UI_LINE_HEIGHT, "Show passworded" );
    showinstagib = Factory::newButton( filter, UI_GAP_WIDTH, (y2offset += UI_GAP_HEIGHT + LABEL_HEIGHT), FILTER_BUTTON_WIDTH, UI_LINE_HEIGHT, "Show instagib" );
    skill = Factory::newButton( filter, UI_GAP_WIDTH, (y2offset += UI_GAP_HEIGHT + LABEL_HEIGHT), FILTER_BUTTON_WIDTH, UI_LINE_HEIGHT, "All skills" );
    showfull->setClickHandler( filterHandler );
    showempty->setClickHandler( filterHandler );
    showpassworded->setClickHandler( filterHandler );
    showinstagib->setClickHandler( filterHandler );
    skill->setClickHandler( filterHandler );
    
	
    lmaxping = Factory::newLabel( filter, UI_GAP_WIDTH, (y2offset += (UI_GAP_HEIGHT + LABEL_HEIGHT)), FILTER_BUTTON_WIDTH, UI_LINE_HEIGHT, "Max ping:" );
    maxping = Factory::newTextBox( filter, UI_GAP_WIDTH + FILTER_BUTTON_WIDTH + UI_GAP_WIDTH, y2offset, FILTER_GAMETYPES_WIDTH, UI_LINE_HEIGHT, 6 );

	lmatchedname = Factory::newLabel( filter, UI_GAP_WIDTH, (y2offset += (UI_GAP_HEIGHT + LABEL_HEIGHT)), FILTER_BUTTON_WIDTH, UI_LINE_HEIGHT, "Matched name:" );
	matchedname = Factory::newTextBox( filter, UI_GAP_WIDTH + FILTER_BUTTON_WIDTH + UI_GAP_WIDTH, y2offset, FILTER_GAMETYPES_WIDTH, UI_LINE_HEIGHT, 50 );
    maxping->setValidateHandler( textFilterHandler );
    matchedname->setValidateHandler( textFilterHandler );

	y2offset = UI_GAP_HEIGHT + UI_LINE_HEIGHT + UI_GAP_HEIGHT;
    gametype = Factory::newListBox( filter, (UI_GAP_WIDTH + FILTER_BUTTON_WIDTH + UI_GAP_WIDTH ) , y2offset, FILTER_GAMETYPES_WIDTH, (4 * UI_GAP_HEIGHT) + (5 * UI_LINE_HEIGHT), false );
    Factory::newListItem( gametype, "All gametypes" );
    for ( int i = 0 ; i < GAMETYPE_NB ; ++i )
	    Factory::newListItem( gametype, gametype_names[i][0] );
	gametype->setItemSelectedHandler( gametypeFilterHandler );
	
	//float x,y, h, w, tmp1, tmp2;
	gametype->getPosition(x, y);
	w = x + gametype->getWidth() + UI_GAP_WIDTH;
	matchedname->getPosition(x, y);
	h = y + UI_GAP_HEIGHT * 2 + matchedname->getHeight();
	filter->setSize(w, h);

	refresh = Factory::newButton( panel, 500, yoffset, 100, 30, "Refresh list" );
    refresh->setClickHandler( refreshHandler );
    yoffset += 40;

    connect = Factory::newButton( panel, 500, yoffset, 100, 30, "Connect" );
    connect->setClickHandler( connectHandler );
    yoffset += 40;
	
	yoffset += 40;
    back = Factory::newButton( panel, 500, yoffset, 100, 30, "Back" );
    back->setClickHandler( backHandler );
}
void JoinServerMenu::serverItemSelected(ListItem *item, int itempos, bool sel)
{
	if (sel) {
		ServerInfo *si;
		if ( (si = (ServerInfo *)(item->getUserData() )) ) {
			Q_strncpyz(joinserver->last_sel, si->address , sizeof(joinserver->last_sel));
		}
	}

	/*if (sel) {
		item->setSize(item->getWidth(), item->getHeight() * 3);
	} else {
		item->setSize(item->getWidth(), item->getHeight() / 3);
	}
	*/
}

//==================
//SearchGames
//==================
void JoinServerMenu::SearchGames( const char *s )
{
    const char	*mlist;
    char		*master;
    int		ignore_empty = 0;
    int		ignore_full = 0;

#ifdef PING_ONCE
    // clear all ping values so they are retried
    ResetAllServers();
#endif

    //"show all", "is", "is not", 0
/*	TODO : check what to ignore
    if( menuitem_emptyfilter )
	    ignore_empty = (menuitem_emptyfilter->curvalue == 2);
    if( menuitem_fullfilter )
	    ignore_full = (menuitem_fullfilter->curvalue == 2);
*/
    mlist = Trap::Cvar_String("masterservers");

/* TODO : show somewhere when no master defined
    if( !mlist || !(*mlist) ) {
	    Menu_SetStatusBar( &s_joinserver_menu, "No master server defined" );
	    return;
    }
*/
    nextServerTime = Local::getTime();
    pingingServer = NULL;

    if( Q_stricmp( s, "local" ) == 0 )
    {
	    // send out request
	    Trap::Cmd_ExecuteText( EXEC_APPEND, va("requestservers %s full empty\n", s));
	    return;
    }
    else if( Q_stricmp( s, "favorites" ) == 0 )
    {
	    int total_favs, i;
	    nextServerTime = Local::getTime();
	    //requestservers local is a hack since favorites is unsupported,
	    //but allows ping responses into the menu.
	    Trap::Cmd_ExecuteText( EXEC_APPEND, va("requestservers local %s %s\n",
		    ignore_full ? "" : "full",
		    ignore_empty ? "" : "empty" ) );
	    total_favs = Trap::Cvar_Int( "favorites" );
	    for( i = 1; i <= total_favs; i++ ) {
		    Trap::Cmd_ExecuteText( EXEC_APPEND, va( "pingserver %s\n", Trap::Cvar_String( va( "favorite_%i", i ) ) ) );
	    }
	    return;
    }
    else
    { //its global
	    // request to each ip on the list
	    while( mlist )
	    {
		    master = COM_Parse( &mlist );
		    if ( !master || !(*master) )
			    break;

		    // send out request
		    Trap::Cmd_ExecuteText( EXEC_APPEND, va("requestservers %s %s %s full empty\n",
			    s, master, Trap::Cvar_String( "gamename" ) ) );
		}
	}
}

void JoinServerMenu::DoubleClickItemHandler(BaseObject *tgt)
{
	connectHandler(NULL);
}

void JoinServerMenu::ClickItemHandler(BaseObject *tgt)
{
}

bool JoinServerMenu::checkFilters( ServerInfo *s )
{
	bool add = true;
	bool filterable;
	int val_gametype;

	// ignore if full (local)
	filterable = s->curuser && s->curuser == s->maxuser;
	if( Trap::Cvar_Value( "ui_filter_full" ) == 1 && !filterable ) // show only
		add = false;
	else if ( Trap::Cvar_Value( "ui_filter_full" ) == 2 && filterable ) // don't show
		add = false;

	// ignore if empty (local)
	filterable = s->curuser == 0;
	if( Trap::Cvar_Value( "ui_filter_empty" ) == 1 && !filterable ) // show only
		add = false;
	else if ( Trap::Cvar_Value( "ui_filter_empty" ) == 2 && filterable ) // don't show
		add = false;

	// ignore if it has password and we are filtering those
	filterable = s->password != 0;
	if( Trap::Cvar_Value( "ui_filter_password" ) == 1 && !filterable ) // show only
		add = false;
	else if ( Trap::Cvar_Value( "ui_filter_password" ) == 2 && filterable ) // don't show
		add = false;

	// ignore if it's of filtered ping
	filterable = s->ping > Trap::Cvar_Value( "ui_filter_ping" );
	if( Trap::Cvar_Value( "ui_filter_ping" ) > 0 && filterable ) // show only
		add = false;

	// ignore if it's name does not match
	if( strlen( Trap::Cvar_String( "ui_filter_name" ) ) )
	{
		char filter_name[80], host_name[80];
		//Remove color and convert to lower case
		Q_strncpyz(filter_name, Trap::Cvar_String("ui_filter_name"), sizeof(filter_name) );
		Q_strncpyz(filter_name, COM_RemoveColorTokens( filter_name), sizeof(filter_name) );
		Q_strlwr( filter_name );

		Q_strncpyz(host_name, Trap::Cvar_String("ui_filter_name"), sizeof(host_name) );
		Q_strncpyz(host_name, COM_RemoveColorTokens( host_name), sizeof(filter_name) );
		Q_strlwr( host_name );
		if( !strstr(host_name, filter_name) )
			add = false;
	}

	// ignore if it's of filtered instagib
	filterable = s->instagib != 0;
	if( Trap::Cvar_Value( "ui_filter_instagib" ) == 1 && !filterable ) // show only
		add = false;
	else if ( Trap::Cvar_Value( "ui_filter_instagib" ) == 2 && filterable ) // don't show
		add = false;

	// ignore if it's of filtered gametype
	val_gametype = Trap::Cvar_Int("ui_filter_gametype");
	// we also check for the case of instagib (idm is dm)
	if ( val_gametype > 0 && (Q_stricmp(gametype_names[val_gametype-1][1], s->gametype.c_str())
			|| (s->gametype[0] == 'i' && Q_stricmp(gametype_names[val_gametype-1][1]+1, s->gametype.c_str()))) )
		add = false;

	// ignore if it's of filtered skill
	filterable = Trap::Cvar_Value( "ui_filter_skill" ) > 0;
	if ( filterable && s->skilllevel && s->skilllevel != Trap::Cvar_Value( "ui_filter_skill" ) )
		add = false;

	return add;
}


template <class Iter> void JoinServerMenu::FillListBoxWithMap( Iter begin, Iter end )
{
	ServerInfo *ptr;
	ListItem *item = NULL, *newselitem = NULL;
	int xoffset;
	int oldnbitem;
	Label *l;
	std::list<ListItem*>::iterator listiter;
	std::list<BaseObject*>::iterator col;

	oldnbitem = serverlist->getNbItem();
	listiter = serverlist->getFirstItemIterator();
	serverlist->clearSelection();
	for ( Iter it = begin ; it != end ; ++it )
	{
		ptr = it->second;
		if( checkFilters( ptr ) )
		{
			if ( oldnbitem > 0 )
			{ // that's not very clean but optimize well (at least i guess so)
				col = (*listiter)->getFirstChildIterator();
				static_cast<Label*>(*col)->setCaption( va("%d", ptr->ping) ); ++col;
				if (ptr->bots) {
					static_cast<Label*>(*col)->setCaption( va("%d/%d (%d bot%s)", (ptr->curuser - ptr->bots), ptr->maxuser, ptr->bots, ((ptr->bots == 1) ? "" : "s")) ); ++col;
				} else {
					static_cast<Label*>(*col)->setCaption( va("%d/%d", ptr->curuser, ptr->maxuser) ); ++col;
				}

				if ( ptr->instagib )
					static_cast<Label*>(*col)->setCaption( va("i%s", ptr->gametype.c_str()) );
				else
					static_cast<Label*>(*col)->setCaption( ptr->gametype );
				++col;
				static_cast<Label*>(*col)->setCaption( ptr->map ); ++col;
				static_cast<Label*>(*col)->setCaption( ptr->hostname );
				//static_cast<Label*>(*col)->setCaption( ptr->address );
				(*listiter)->setUserData(ptr);
				(*listiter)->setPressed(false);
				if ( (last_sel[0]) && ( !Q_strnicmp(last_sel, ptr->address, sizeof(last_sel))) ) {
					newselitem = (*listiter);
				}
				oldnbitem--;
				++listiter;
			}
			else
			{
				xoffset = 0;
				item = Factory::newListItem( serverlist );
				l = Factory::newLabel( item, 0, 0, PING_COL_WIDTH, 20, va("%d", ptr->ping) );
				l->setAlign( ALIGN_TOP_CENTER );
				l->setDisabledFontColor( l->getFontColor() );
				l->setEnabled( true );
				l->setClickThrough( true );
				if (ptr->bots) {
					l = Factory::newLabel( item, xoffset += PING_COL_WIDTH, 0, PLAYER_COL_WIDTH, 20, va("%d/%d (%d bot%s)", (ptr->curuser - ptr->bots), ptr->maxuser, ptr->bots, ((ptr->bots == 1) ? "" : "s") ) );
				} else {
					l = Factory::newLabel( item, xoffset += PING_COL_WIDTH, 0, PLAYER_COL_WIDTH, 20, va("%d/%d", ptr->curuser, ptr->maxuser) );
				}
				
				l->setAlign( ALIGN_TOP_CENTER );
				l->setDisabledFontColor( l->getFontColor() );
				l->setEnabled( true );
				l->setClickThrough( true );

				if ( ptr->instagib )
					l = Factory::newLabel( item, xoffset += PLAYER_COL_WIDTH, 0, GAMETYPE_COL_WIDTH, 20, va("i%s", ptr->gametype.c_str()) );
				else
					l = Factory::newLabel( item, xoffset += PLAYER_COL_WIDTH, 0, GAMETYPE_COL_WIDTH, 20, ptr->gametype );
				l->setAlign( ALIGN_TOP_CENTER );
				l->setDisabledFontColor( l->getFontColor() );
				l->setEnabled( true );
				l->setClickThrough( true );

				l = Factory::newLabel( item, xoffset += GAMETYPE_COL_WIDTH, 0, MAP_COL_WIDTH, 20, ptr->map );
				l->setAlign( ALIGN_TOP_CENTER );
				l->setDisabledFontColor( l->getFontColor() );
				l->setEnabled( true );
				l->setClickThrough( true );

				l = Factory::newLabel( item, xoffset += MAP_COL_WIDTH, 0, NAME_COL_WIDTH, 20, ptr->hostname );
				l->setAlign( ALIGN_TOP_LEFT );
				l->setDisabledFontColor( l->getFontColor() );
				l->setEnabled( true );
				l->setClickThrough( true );

				item->setUserData(ptr);
				item->setClickHandler(ClickItemHandler);
				item->setDoubleClickHandler(DoubleClickItemHandler);
				item->setPressed(false);
				if ( (last_sel[0]) && ( !Q_strnicmp(last_sel, ptr->address, sizeof(last_sel))) ) {
					newselitem = item;
				}
				listiter = serverlist->getEndItemIterator();
			}
		}
	}
	if ( listiter != serverlist->getEndItemIterator() )
	{
		//while (
		for ( int i = serverlist->getItemPosition( *listiter ) ; i < serverlist->getNbItem() ; serverlist->removeItem( i ) );
		
	}
	if (newselitem) {
		// Restore previously selected server or select first item in the list
		serverlist->selectItem(newselitem);
		serverlist->ensureItemVisible(newselitem, 2);	
	}
}

void JoinServerMenu::RefreshServerList( void )
{ // I still can't say if I love templates or if I hate them... (but I think I hate them)
	if ( sorttype == SORT_PING && !reverse_sort )
		FillListBoxWithMap( pingSortMap.begin(), pingSortMap.end() );
	else if ( sorttype == SORT_PING && reverse_sort )
		FillListBoxWithMap( pingSortMap.rbegin(), pingSortMap.rend() );
	else if ( sorttype == SORT_PLAYER && !reverse_sort )
		FillListBoxWithMap( playerSortMap.begin(), playerSortMap.end() );
	else if ( sorttype == SORT_PLAYER && reverse_sort )
		FillListBoxWithMap( playerSortMap.rbegin(), playerSortMap.rend() );
	else if ( sorttype == SORT_MAP && !reverse_sort )
		FillListBoxWithMap( mapSortMap.begin(), mapSortMap.end() );
	else if ( sorttype == SORT_MAP && reverse_sort )
		FillListBoxWithMap( mapSortMap.rbegin(), mapSortMap.rend() );
	else if ( sorttype == SORT_GAMETYPE && !reverse_sort )
		FillListBoxWithMap( gametypeSortMap.begin(), gametypeSortMap.end() );
	else if ( sorttype == SORT_GAMETYPE && reverse_sort )
		FillListBoxWithMap( gametypeSortMap.rbegin(), gametypeSortMap.rend() );
	else if ( sorttype == SORT_NAME && !reverse_sort )
		FillListBoxWithMap( nameSortMap.begin(), nameSortMap.end() );
	else if ( sorttype == SORT_NAME && reverse_sort )
		FillListBoxWithMap( nameSortMap.rbegin(), nameSortMap.rend() );
}

void JoinServerMenu::AddToServerList( char *adr, char *info )
{
	ServerInfo *newserv = NULL;
	std::list<ServerInfo*>::iterator tmp;

	if ( !joinserver->refreshingList ) //User stopped the listing
		return;

	// check if server is already in the list
	for ( std::list<ServerInfo*>::iterator it = joinserver->servers.begin() ; it != joinserver->servers.end() ; )
	{
		if (! ((*it)->isPingBatch(joinserver->pingbatch)) ) {
			// Clean up previous ping-batches
			tmp = it;
			it++;
			delete(*tmp);
			joinserver->servers.erase(tmp);
			continue;
		}
		if ( Q_stricmp( (*it)->address, adr ) == 0 )
		{
			newserv = *it;
			break;
		}
		it++;
	}
	if ( !newserv )
	{
		newserv = new ServerInfo( adr, joinserver->pingbatch );
		joinserver->servers.push_back( newserv );
	}

	newserv->setServerInfo( info );

	if( newserv->hasChanged() )
	{
		if ( newserv->pingUpdated() )
		{
			// sorting
			joinserver->pingSortMap.insert( std::make_pair( newserv->ping, newserv ) );
			joinserver->playerSortMap.insert( std::make_pair( newserv->curuser, newserv ) );
			joinserver->mapSortMap.insert( std::make_pair( newserv->map, newserv ) );
			joinserver->gametypeSortMap.insert( std::make_pair( newserv->gametype, newserv ) );
			joinserver->nameSortMap.insert( std::make_pair( std::string(COM_RemoveColorTokens(newserv->hostname.c_str())), newserv ) );
			joinserver->pingingServer = NULL;
			joinserver->RefreshServerList();
		}
		newserv->changeUpdated();
	}
}

void JoinServerMenu::StyleArrow( Panel *arr, SwitchButton *sorter, bool reverse )
{
	arr->setSize(10,10);
    arr->setEnabled( false );
    arr->setDisabledColor( sorter->getPressedFontColor() );
    arr->setBorderWidth( 0 );
	arr->setDisabledImage( Trap::R_RegisterPic( reverse ? "gfx/ui/arrow_up" : "gfx/ui/arrow_down") );
    arr->setParent( sorter );
    arr->setPosition( sorter->getWidth() - 10, 5 );
	arr->setVisible(true);
}
void JoinServerMenu::StyleSorter( SwitchButton *sorter )
{
	Color transparent( 0, 0, 0, 0 );
	sorter->setBackColor( transparent );
	sorter->setPressedColor( transparent );
	sorter->setSwitchHandler( sortHandler );
}

void JoinServerMenu::setFilter( Button *button, int filter, const char *filtername )
{
	if ( filter == 1 )
		button->setCaption( va( "Show %s only", filtername ) );
	else if ( filter == 2 )
		button->setCaption( va( "Do not show %s", filtername ) );
	else
		button->setCaption( va( "Show %s", filtername ) );
}

void JoinServerMenu::LoadUserSettings( void )
{
	int filter;

	setFilter( showfull, Trap::Cvar_Int( "ui_filter_full" ), "full" );
	setFilter( showempty, Trap::Cvar_Int( "ui_filter_empty" ), "empty" );
	setFilter( showpassworded, Trap::Cvar_Int( "ui_filter_password" ), "passworded" );
	setFilter( showinstagib, Trap::Cvar_Int( "ui_filter_instagib" ), "instagib" );

	filter = Trap::Cvar_Int( "ui_filter_skill" );
	if ( filter == 1 )
		skill->setCaption( "Low" );
	else if ( filter == 2 )
		skill->setCaption( "Medium" );
	else if ( filter == 3 )
		skill->setCaption( "High" );
	else
		skill->setCaption( "Show all skills" );

	filter = Trap::Cvar_Int( "ui_filter_ping" );
	if ( filter )
		maxping->setText( va( "%d", filter ) );

	matchedname->setText( Trap::Cvar_String( "ui_filter_name" ) );
	gametype->selectItem( Trap::Cvar_Int( "ui_filter_gametype" ) );

	RefreshServerList();

/*	
trap_Cvar_Value("ui_searchtype") );
trap_Cvar_Value("cl_download_allow_modules") );
trap_Cvar_Value("ui_sortmethod_direction") );
trap_Cvar_Value("ui_sortmethod");
"Master server at %s", trap_Cvar_VariableString("masterservers") );
*/

}

void JoinServerMenu::SaveUserSettings( void )
{

}

void JoinServerMenu::Show( void )
{
	setActiveMenu( this );
	panel->setVisible( true );

	LoadUserSettings();
	RefreshServerList();
	if (servers.size() == 0) {
		refreshHandler(NULL);
	}
}

void JoinServerMenu::Hide( void )
{
	panel->setVisible( false );
}
