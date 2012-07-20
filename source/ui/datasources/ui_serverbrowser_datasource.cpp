#include "ui_precompiled.h"
#include "kernel/ui_common.h"
#include "kernel/ui_main.h"
#include "kernel/ui_utils.h"

#include <sstream>
#include <iterator>
#include "datasources/ui_serverbrowser_datasource.h"

namespace WSWUI {

// TODO: constify
#define TABLE_NAME_NORMAL	"normal"
#define TABLE_NAME_INSTA	"instagib"
#define TABLE_NAME_TV		"tv"
#define TABLE_NAME_FAVORITES "favorites"

//=====================================

namespace {

	void DEBUG_PRINT_SERVERINFO( const ServerInfo &info )
	{
		Com_Printf("^6Serverinfo:\n%s %s %s %d/%d %s %s %d %d %d %d %d\n",
					info.address.c_str(), info.hostname.c_str(), info.map.c_str(),
					info.curuser, info.maxuser, info.gametype.c_str(),
					info.modname.c_str(), int(info.instagib), info.skilllevel,
					int(info.password), int(info.mm), info.ping );
	}

	// TODO: move this as general utility

}

//=====================================

// ServerInfo
ServerInfo::ServerInfo( const char *adr, const char *info )
	:	has_changed(false), ping_updated(false), address( adr ),
		iaddress( addr_to_int( adr ) ), hostname(""), cleanname(""), map(""), curuser(0),
		maxuser(0), bots(0), gametype(""), modname(""), instagib(false), skilllevel(0),
		password(false), mm(false), tv(false), ping(0), ping_retries(0), favorite(false)
{
	if( info )
		fromInfo( info );
}

ServerInfo::ServerInfo( const ServerInfo &other )
{
	fromOther( other );
}

ServerInfo &ServerInfo::operator=( const ServerInfo &other )
{
	fromOther( other );
	return *this;
}

void ServerInfo::fromOther( const ServerInfo &other )
{
	has_changed = other.has_changed;
	ping_updated = other.ping_updated;
	address = other.address;
	iaddress = other.iaddress;
	hostname = other.hostname;
	cleanname = other.cleanname;
	locleanname = other.locleanname;
	map = other.map;
	curuser = other.curuser;
	maxuser = other.maxuser;
	bots = other.bots;
	gametype = other.gametype;
	modname = other.modname;
	instagib = other.instagib;
	skilllevel = other.skilllevel;
	password = other.password;
	mm = other.mm;
	tv = other.tv;
	ping = other.ping;
	ping_retries = other.ping_retries;
	favorite = other.favorite;
}

void ServerInfo::fromInfo( const char *info )
{
	__stl_vector(std::string) tokens;
	__stl_vector(std::string)::iterator it;

	tokenizeInfo( info, tokens );

	for( it = tokens.begin(); it != tokens.end(); it++ /* advanced 1 time inside loop too */ )
	{
		// need to trim this?
		const std::string &cmd = *it;
		if(cmd == "EOT")
			break;

		// advance to next token and check for premature exit
		if( *(++it) == "EOT" || it == tokens.end() )
			break;	// WARN?

		const std::string &value = *it;

		if( cmd == "n" ) {	// HOSTNAME
			// TODO: sortname!
			if( hostname != value ) {
				has_changed = true;
				hostname = value;

				// hostname with color sequences stripped
				cleanname = COM_RemoveColorTokensExt( value.c_str(), qfalse );

				// lowercase stripped version of non-colored name, used for sorting
				locleanname = cleanname;
				std::transform( locleanname.begin(), locleanname.end(), locleanname.begin(), ::tolower );

				// fails on utf-8 strings
				//locleanname.erase( std::remove_if(locleanname.begin(), locleanname.end(), std::not1(std::ptr_fun(::isalnum))), locleanname.end() );
			}
		}
		else if( cmd == "m" ) {	// MAP NAME
			std::string tmpmap;
			std::stringstream trim( value );
			trim >> tmpmap;
			if( tmpmap != map ) {
				has_changed = true;
				map = tmpmap;
			}
		}
		else if( cmd == "u" ) {	// USERS / MAXUSERS
			int tmpcur, tmpmax;
			char dummy;
			std::stringstream parser( value );
			parser >> tmpcur >> dummy >> tmpmax;
			if( !parser.fail() && (tmpcur != curuser || tmpmax != maxuser) ) {
				has_changed = true;
				curuser = tmpcur;
				maxuser = tmpmax;
			}
		}
		else if( cmd == "b" ) {	// BOT COUNT
			int tmpbots;
			std::stringstream toint( value );
			toint >> tmpbots;
			if( !toint.fail() && tmpbots != bots ) {
				has_changed = true;
				bots = tmpbots;
			}
		}
		else if( cmd == "g" ) {	// GAMETYPE
			std::string tmpgame;
			std::stringstream trim( value );
			trim >> tmpgame;
			if( !trim.fail() && tmpgame != gametype ) {
				has_changed = true;
				gametype = tmpgame;
			}
		}
		else if( cmd == "ig" ) { // INSTAGIB
			int tmpinsta;
			std::stringstream toint( value );
			toint >> tmpinsta;
			if( !toint.fail() && (tmpinsta != 0) != instagib ) {
				has_changed = true;
				instagib = tmpinsta != 0;
			}
		}
		else if( cmd == "s" ) { // SKILL LEVEL
			int tmpskill;
			std::stringstream toint( value );
			toint >> tmpskill;
			if( !toint.fail() && tmpskill != skilllevel ) {
				has_changed = true;
				skilllevel = tmpskill;
			}
		}
		else if( cmd == "p" ) { // PASSWORD
			int tmppass;
			std::stringstream toint( value );
			toint >> tmppass;
			if( !toint.fail() && (tmppass != 0) != password ) {
				has_changed = true;
				password = tmppass != 0;
			}
		}
		else if( cmd == "ping" ) { // PING.. doh
			unsigned int tmpping;
			std::stringstream toint( value );
			toint >> tmpping;
			if( !toint.fail() && ( tmpping != ping || ping_retries == 0 ) ) {
				has_changed = true;
				ping_updated = true;
				ping = tmpping;
				if( ping > 999 )
					ping = 999;
			}
		}
		else if( cmd == "mm" ) { // MATCHMAKING
			int tmpmm;
			std::stringstream toint( value );
			toint >> tmpmm;
			if( !toint.fail() && (tmpmm != 0) != mm ) {
				has_changed = true;
				mm = tmpmm != 0;
			}
		}
		else if( cmd == "mo" ) { // MOD NAME
			std::string tmpmod;
			std::stringstream trim( value );
			trim >> tmpmod;
			if( !trim.fail() && tmpmod != modname ) {
				has_changed = true;
				modname = tmpmod;
			}
		}
		else if( cmd == "tv" ) { // TV SERVER
			int tmptv;
			std::stringstream toint( value );
			toint >> tmptv;
			if( !toint.fail() && (tmptv != 0) != tv ) {
				has_changed = true;
				tv = tmptv != 0;
			}
		}
		else {
			Com_Printf( "ServerInfo::fromInfo(%s): Unknown token:\"%s\"\n", info, cmd.c_str() );
			// take the value token back
			it--;
		}
	}

	// subtract bots from users
	if( has_changed ) {
		curuser = std::max( 0, curuser - bots );
	}
}

// TODO: remove, redundant cause we have tokenize
void ServerInfo::tokenizeInfo( const char *info, __stl_vector(__stl_string) &tokens )
{
	__stl_string str( info );
	size_t len, left, right = 0;

	while( right != __stl_string::npos ) {
		left = str.find_first_not_of( '\\', right );
		if( left == __stl_string::npos )
			break;
		right = str.find_first_of( '\\', left );
		// fix length
		len = ( right == __stl_string::npos ? __stl_string::npos : (right - left) );
		tokens.push_back( str.substr( left, len ) );
	}

#if 0
    Com_Printf("ServerInfo::tokenizeInfo: %d tokens from %s\n\t", tokens.size(), info );
    for( __stl_vector(__stl_string)::iterator it = tokens.begin(); it != tokens.end(); it++ )
		Com_Printf("%s, ", it->c_str() );
	Com_Printf("\n");
#endif
}

// TODO: do this as a DataFormatter
// html encode single string inplace
void ServerInfo::htmlEncode( std::string &s )
{
	// &<>"
	std::string::size_type pos = s.find( '&' );
	while( pos != std::string::npos ) {
		s.replace( pos, 1, "&amp;" );
		pos = s.find( '&', pos+1 );
	}
	pos = s.find( '<' );
	while( pos != std::string::npos ) {
		s.replace( pos, 1, "&lt;" );
		pos = s.find( '<', pos+1 );
	}
	pos = s.find( '>' );
	while( pos != std::string::npos ) {
		s.replace( pos, 1, "&gt;" );
		pos = s.find( '>', pos+1 );
	}
	// rocket knows no &quot;
	/*
	pos = s.find( '"' );
	while( pos != std::string::npos ) {
		s.replace( pos, 1, "&quot;" );
		pos = s.find( '"', pos+1 );
	}
	*/
}

// html encode all string fields, fix color tags and
// also avoid empty strings or they cause weird problems
// with libFocket
void ServerInfo::fixStrings()
{
	htmlEncode( hostname );
	if( !hostname.length() )
		hostname = "&nbsp;";

	htmlEncode( cleanname );
	if( !cleanname.length() )
		cleanname = "&nbsp;";

	htmlEncode( map );
	if( !map.length() )
		map = "&nbsp;";

	htmlEncode( gametype );
	if( !gametype.length() )
		gametype = "&nbsp;";

	htmlEncode( modname );
	if( !modname.length() )
		modname = "&nbsp;";
}

// default comparison function for servers:
// 1) No of players DESC
// 2) ping ASC
// 3) hostname ASC
bool ServerInfo::DefaultCompareBinary( const ServerInfo *lhs, const ServerInfo *rhs )
{
	if( lhs->curuser > rhs->curuser ) return true;
	if( lhs->curuser < rhs->curuser ) return false;

	if( lhs->ping < rhs->ping ) return true;
	if( lhs->ping > rhs->ping ) return false;

	return LessPtrBinary<std::string, &ServerInfo::locleanname>( lhs, rhs );
}

//=====================================

// ServerInfoFetcher

// add a query to the waiting line
void ServerInfoFetcher::addQuery( const char *adr )
{
//	Com_Printf("addQuery %s\n", adr );
//	if( numQueries() < MAX_QUERIES )
//	{
//		startQuery( adr );
//		return;
//	}

	// else
	serverQueue.push( adr );
}

// called to tell fetcher to remove this from jobs
void ServerInfoFetcher::queryDone( const char *adr )
{
	// remove from active list
	ActiveList::iterator it = std::find_if(
			activeQueries.begin(), activeQueries.end(), CompareAddress(adr) );

	if( it != activeQueries.end() )
		activeQueries.erase( it );

	// this slot will be filled in update
	// Com_Printf("queryDone %s\n", adr );
}

// stop the whole process
void ServerInfoFetcher::clearQueries()
{
	activeQueries.clear();
	while( numWaiting() > 0 )
		serverQueue.pop();
}

// advance queries
void ServerInfoFetcher::updateFrame()
{
	unsigned int now = trap::Milliseconds();
	unsigned int treshold = now - (TIMEOUT_SEC * 1000);
	ActiveList::iterator it;

	// remove old items (timeout)
	for( it = activeQueries.begin(); it != activeQueries.end(); )
	{
		if( it->first < treshold )
		{
			// Com_Printf("timeout: %s %u %u\n", it->second.c_str(), it->first, treshold );
			// we should notify serverBrowser here about timeout
			it = activeQueries.erase( it );
		}
		else {
			it++;
		}
	}

	// populate active queries with ones on the waiting line
	if( now > lastQueryTime + QUERY_TIMEOUT_MSEC && numWaiting() > 0 )
	{
		lastQueryTime = now;
		startQuery( serverQueue.front() );
		serverQueue.pop();
	}
}

// initiates a query
void ServerInfoFetcher::startQuery( const std::string &adr )
{
	// add to the active list
	activeQueries.push_back( std::make_pair( trap::Milliseconds(), adr ) );

	// execute command to initiate the query
	trap::Cmd_ExecuteText( EXEC_APPEND, va( "pingserver %s;", adr.c_str() ) );

//	Com_Printf("startQuery: %s\n", adr.c_str() );
}

//=====================================

// ServerBrowserDataSource

ServerBrowserDataSource::ServerBrowserDataSource() :
		Rocket::Controls::DataSource("serverbrowser_source"),
		serverList(),
		fetcher(this), active(false)
{
	// default sorting function
	// hostname
	sortCompare = &ServerInfo::DefaultCompareBinary;
	active = false;

	referenceListMap.clear();

	// DEBUG
	numNotifies = 0;
}

ServerBrowserDataSource::~ServerBrowserDataSource()
{
	referenceListMap.clear();
}

// override rocket methods
void ServerBrowserDataSource::GetRow( StringList &row, const String &table, int row_index, const StringList &columns )
{
	// DEBUG
	// Com_Printf("ServerBrowserDataSource::GetRow %s %d ..\n", table.CString(), row_index );

	if( referenceListMap.find( table ) == referenceListMap.end() ) 
		return;

	ReferenceList &list = referenceListMap[table];
	if( row_index < 0 || (size_t)row_index >= list.size() )
		return;

	// TODO: use referenceList to obtain the row
	ReferenceList::iterator it_info = list.begin();
	std::advance( it_info, row_index );

	// bogg out?
	if( it_info == list.end() )
		return;

	const ServerInfo &info = *(*it_info);
	for( StringList::const_iterator it = columns.begin(); it != columns.end(); it++ )
	{
		// TODO: htmlencode here! "<>& etc..
		if( *it == "hostname" )
			row.push_back( info.hostname.c_str() );
		else if( *it == "cleanname" )
			row.push_back( info.cleanname.c_str() );
		else if( *it == "map" )
			row.push_back( info.map.c_str() );
		else if( *it == "players" )
			row.push_back( va( "%d/%d", info.curuser, info.maxuser) );
		else if( *it == "bots" )
			row.push_back( va( "%d", info.bots ) );
		else if( *it == "gametype" )
			row.push_back( info.tv ? "TV" : info.gametype.c_str() );
		else if( *it == "instagib" )
			row.push_back( info.instagib ? "yes" : "no" );
		else if( *it == "skilllevel" )
			row.push_back( va( "%d", info.skilllevel ) );
		else if( *it == "password" )
			row.push_back( info.password ? "yes" : "no" );
		else if( *it == "mm" )
			row.push_back( info.mm ? "yes" : "no" );
		else if( *it == "ping" )
			row.push_back( va( "%d", info.ping ) );
		else if( *it == "address" )
			row.push_back( info.address.c_str() );
		else if( *it == "tv" )
			row.push_back( info.tv ? "yes" : "no" );
		else if( *it == "favorite" )
			row.push_back( info.favorite ? "yes" : "no" );
		else if( *it == "mod" )
			row.push_back( info.modname.c_str() );
		else
			// Com_Printf("ServerBrowserDataSource::GetRow: unknown column %s\n", it->CString() );
			row.push_back( "" );
	}
}

// this should return the number of rows in 'table'
int ServerBrowserDataSource::GetNumRows( const String &table )
{
	if( referenceListMap.find( table ) == referenceListMap.end() ) 
		return 0;
	return referenceListMap[table].size();
}

void ServerBrowserDataSource::tableNameForServerInfo( const ServerInfo &info, String &table ) const
{
	if( info.tv ) {
		table = TABLE_NAME_TV;
	}
	else if( info.instagib ) {
		table = TABLE_NAME_INSTA;
	}
	else {
		table = TABLE_NAME_NORMAL;
	}
}

void ServerBrowserDataSource::addServerToTable( ServerInfo &info, String tableName )
{
	ReferenceList &referenceList = referenceListMap[tableName];

	// Show/sort with referenceList
	ReferenceList::iterator it = find_if( referenceList,
							ServerInfo::EqualUnary<quint64, &ServerInfo::iaddress>( info.iaddress ) );

	if( it == referenceList.end() ) {
		// server isnt in the list, use insertion sort to put it in

		// insertion sort (FIXME: batchAdd has to batch adjacent elements!)
		it = referenceList.insert( lower_bound( referenceList, &info, sortCompare ), &info );

		// notify rocket on the addition of row
		NotifyRowAdd( tableName, std::distance( referenceList.begin(), it ) /*referenceList.size()-1 */, 1 );
	}
	else {
		// notify rocket on the change of a row
		NotifyRowChange( tableName, std::distance( referenceList.begin(), it ), 1 );
	}
}

void ServerBrowserDataSource::removeServerFromTable( ServerInfo &info, String tableName )
{
	ReferenceList &referenceList = referenceListMap[tableName];

	// notify rocket + remove from referenceList
	ReferenceList::iterator it = find_if( referenceList,
							ServerInfo::EqualUnary<quint64, &ServerInfo::iaddress>( info.iaddress ) );

	if( it != referenceList.end() ) {
		int index = std::distance( referenceList.begin(), it );
		referenceList.erase( it );
		NotifyRowRemove( tableName, index, 1 );
	}
}

// called each frame to progress queries
void ServerBrowserDataSource::updateFrame()
{
	// ch : removed de-activation by time

	// DEBUG:
	numNotifies = 0;

	// query queue
	fetcher.updateFrame();

	// incoming info queue
	while( referenceQueue.size() > 0 )
	{
		ServerInfo &serverInfo = *(referenceQueue.front());
		// yes this is safe, its only a pointer
		referenceQueue.pop_front();

		// DEBUG_PRINT_SERVERINFO( serverInfo );

		// put to the visible list if it passes the filters
		if( filter.filterServer( serverInfo ) )
		{
			String tableName;
			
			tableNameForServerInfo( serverInfo, tableName );
			addServerToTable( serverInfo, tableName );

			if( serverInfo.favorite )
				addServerToTable( serverInfo, TABLE_NAME_FAVORITES );
		}
	}

	//if( numNotifies )
	//	Com_Printf("ServerBrowser::updateFrame %d notifies\n", numNotifies );

	if( !fetcher.numQueries() && !fetcher.numWaiting() && active && !referenceListMap.empty() ) {
		active = false;
	}
}

// initiates master server query
void ServerBrowserDataSource::startFullUpdate( void )
{
	// ch : removed de-activation by time

	active = true;

	// formulate the query and execute
	// basic prototype:
	// 		requestservers global dpmaster.deathmask.net Warsow full empty
	// TODO: implement proper use of filters...
	
	// might not want to do this?
	serverList.clear();

	std::vector<std::string> masterServers;
	tokenize( trap::Cvar_String("masterservers"), ' ', masterServers );

	for( std::vector<std::string>::iterator it = masterServers.begin(); it != masterServers.end(); it++ ) {
		std::string queryString =  std::string ("requestservers global ") + *it + " Warsow full empty";

		trap::Cmd_ExecuteText( EXEC_APPEND, queryString.c_str() );
	}

	// query for LAN servers too
	trap::Cmd_ExecuteText( EXEC_APPEND, "requestservers local" );

	for( ReferenceListMap::iterator it = referenceListMap.begin(); it != referenceListMap.end(); it++ ) {
		size_t size = it->second.size();
		if( size > 0 ) {
			it->second.clear();
			NotifyRowRemove( it->first, 0, size );
		}
	}

	referenceListMap.clear();
}

// callback from client propagates to here
void ServerBrowserDataSource::addToServerList( const char *adr, const char *info )
{
	// check if user has canceled the update
	if( !active )
		return;

	// TODO: hint for the address. If previous address matches this address
	// use that previous iterator as a hint for the insert function!


	// create serverinfo object and associate with the list
	ServerInfo newInfo( adr, info );
	ServerInfoListPair it_inserted = serverList.insert( newInfo );
	ServerInfo &serverInfo( const_cast<ServerInfo&>( *it_inserted.first ) );

	// check if we have to copy the new item
	if( !it_inserted.second )
		serverInfo = newInfo;

	// -- we could also just match \\EOT in info --
	// initial addition from master query or ping error
	if( it_inserted.second || !serverInfo.isChanged() )
	{
		// check if we want to drop this
		if( serverInfo.ping_retries++ >= MAX_RETRIES )
		{
			String tableName;
			
			tableNameForServerInfo( serverInfo, tableName );

			// drop the query
			fetcher.queryDone( adr );

			// notify rocket + remove from referenceList
			removeServerFromTable( serverInfo, tableName );

			if( serverInfo.favorite ) {
				removeServerFromTable( serverInfo, TABLE_NAME_FAVORITES );
			}

			// and finally from the main list
			serverList.erase( it_inserted.first );
			return;
		}
		else
		{
			// drop the possible query
			fetcher.queryDone( adr );

			// push to waitingLine
			fetcher.addQuery( adr );
		}
	}
	else	// woohoo, we have actual data
	{
		// clear retries
		serverInfo.ping_retries = 0;

		// tell the serverinfo fetcher to remove this item
		fetcher.queryDone( adr );

		// process the string fields
		serverInfo.fixStrings();

		// Com_Printf( it_inserted.second ? "Inserting " : "Changing " );
		// DEBUG_PRINT_SERVERINFO( serverInfo );

		FavoritesList::const_iterator it = favorites.find( serverInfo.iaddress );
		if( it != favorites.end() ) {
			serverInfo.favorite = true;
		}

		referenceQueue.push_back( &serverInfo );
	}

	// finally mark serverinfo unchanged
	serverInfo.setChanged( false );
}

// refreshes (pings) current list
void ServerBrowserDataSource::startRefresh( void )
{
	// ch : removed de-activation by time

	active = true;

	// push serverinfo list to fetcher
	// TODO:
}

// and stop current update/refresh
void ServerBrowserDataSource::stopUpdate( void )
{
	active = false;
	fetcher.clearQueries();
}

// called to re-sort the data
void ServerBrowserDataSource::sortByColumn( const char *_column )
{
	// do a full re-sorting of the data by given column name (lookup from COLUMN_NAMES)
	// (for visibleServers only)

	// set the sorting function
	std::string column( _column );
	if( column == "address" )
		sortCompare = ServerInfo::LessPtrBinary<std::string, &ServerInfo::address>;
	else if( column == "hostname" )
		sortCompare = ServerInfo::LessPtrBinary<std::string, &ServerInfo::hostname>;
	else if( column == "map" )
		sortCompare = ServerInfo::LessPtrBinary<std::string, &ServerInfo::map>;
	else if( column == "players" )
		sortCompare = ServerInfo::LessPtrBinary<int, &ServerInfo::curuser>;
	else if( column == "bots" )
		sortCompare = ServerInfo::LessPtrBinary<int, &ServerInfo::bots>;
	else if( column == "gametype" )
		sortCompare = ServerInfo::LessPtrBinary<std::string, &ServerInfo::gametype>;
	else if( column == "modname" )
		sortCompare = ServerInfo::LessPtrBinary<std::string, &ServerInfo::modname>;
	else if( column == "instagib" )
		sortCompare = ServerInfo::LessPtrBinary<bool, &ServerInfo::instagib>;
	else if( column == "skilllevel" )
		sortCompare = ServerInfo::LessPtrBinary<int, &ServerInfo::skilllevel>;
	else if( column == "password" )
		sortCompare = ServerInfo::LessPtrBinary<bool, &ServerInfo::password>;
	else if( column == "mm" )
		sortCompare = ServerInfo::LessPtrBinary<bool, &ServerInfo::mm>;
	else if( column == "ping" )
		sortCompare = ServerInfo::LessPtrBinary<unsigned int, &ServerInfo::ping>;
	else
	{
		Com_Printf("Serverbrowser sort: unknown column %s\n", _column );
		return;
	}

	// we're not really sorting this one here..
	// serverList.sort( sortCompare );

	// then tell rocket that our table is changed
	//NotifyRowChange(TABLE_NAME);
}

// called to reform visibleServers and hiddenServers
void ServerBrowserDataSource::filtersUpdated(void)
{

}

void ServerBrowserDataSource::notifyOfFavoriteChange( quint64 iaddr, bool add )
{
	// lets see if the server is already in our serverlist
	ServerInfoList::iterator it_s = find_if( serverList,
		ServerInfo::EqualUnary<quint64, &ServerInfo::iaddress>( iaddr ) );

	if( it_s == serverList.end() ) {
		return;
	}

	// hack const_cast in here becase all std::set iterators are const
	// and we really want to update the 'favorite' field, which doesn't
	// affect ordering of the elements
	ServerInfo *info = const_cast<ServerInfo *> ( &(*it_s) );
	info->favorite = add;

	// tell libRocket we've updated the table row
	String tableName;
	tableNameForServerInfo( *it_s, tableName );
	ReferenceList &referenceList = referenceListMap[tableName];

	ReferenceList::iterator it = find_if( referenceList,
							ServerInfo::EqualUnary<quint64, &ServerInfo::iaddress>( iaddr ) );
	if( it != referenceList.end() ) {
		NotifyRowChange( tableName, std::distance( referenceList.begin(), it ), 1 );
	}

	// add server to favorite table
	if( add ) {
		addServerToTable( *info, TABLE_NAME_FAVORITES );
	}
	else {
		removeServerFromTable( *info, TABLE_NAME_FAVORITES );
	}
}

bool ServerBrowserDataSource::addFavorite( const char *fav )
{
	quint64 iaddr = addr_to_int( fav );

	// is that address already favorited?
	FavoritesList::const_iterator it_f = favorites.find( iaddr );
	if( it_f != favorites.end() ) {
		return false;
	}

	favorites.insert( iaddr );

	// update serverInfo and notify libRocket of the change
	notifyOfFavoriteChange( iaddr, true );

	return true;
}

bool ServerBrowserDataSource::removeFavorite( const char *fav )
{
	quint64 iaddr = addr_to_int( fav );

	FavoritesList::const_iterator it_f = favorites.find( iaddr );
	if( it_f == favorites.end() ) {
		return false;
	}
	
	// that server is the one we don't like any longer
	favorites.erase( it_f );

	// update serverInfo and notify libRocket of the change
	notifyOfFavoriteChange( iaddr, false );

	return true;
}

}
