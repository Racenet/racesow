/**
 * Helper functions
 */

/**
 * Print the Message when the Test fails and abort the Gametype by using a nullpointer access (dirty hack!)
 *
 * @param test The Test or Condition which will be used
 * @param msg The Message which is printed when the test fails
 */
void assert( const bool test, const cString msg )
{
	if ( !test )
	{
        cString@ assertstring;
		G_Print( S_COLOR_RED + "assert failed [" + gametype.getName() + " " + gametype.getVersion() + "]: " + "\n" );
        G_Print( S_COLOR_RED + msg + "\n");
        assertstring.length();
	}
}

/**
 * Print the diff string beteween two times.
 * The diff colors change according to which time is best.
 * If the first time is 0 then we consider that the diff doesn't make sense
 * and print dashes instead
 *
 * @param oldTime The old time, the one you compare to
 * @param newTime The new time, the one you want to compare
 * @return The diff string between the two times
 */
cString diffString( uint oldTime, uint newTime )
{
    if ( oldTime == 0 )
    {
        return "--:--:---";
    }
    else if ( oldTime < newTime )
    {
        return S_COLOR_RED + "+" + TimeToString( newTime - oldTime );
    }
    else if ( oldTime == newTime )
    {
        return S_COLOR_YELLOW + "+-" + TimeToString( 0 );
    }
    else
    {
        return S_COLOR_GREEN + "-" + TimeToString( oldTime - newTime );
    }
}

/**
 * TimeToString
 * @param uint time
 * @return cString
 */
cString TimeToString( uint time )
{
    // convert times to printable form
    cString minsString, secsString, millString;
    uint min, sec, milli;

    milli = time;
    min = milli / 60000;
    milli -= min * 60000;
    sec = milli / 1000;
    milli -= sec * 1000;

    if ( min == 0 )
        minsString = "00";
    else if ( min < 10 )
        minsString = "0" + min;
    else
        minsString = min;

    if ( sec == 0 )
        secsString = "00";
    else if ( sec < 10 )
        secsString = "0" + sec;
    else
        secsString = sec;

    if ( milli == 0 )
        millString = "000";
    else if ( milli < 10 )
        millString = "00" + milli;
    else if ( milli < 100 )
        millString = "0" + milli;
    else
        millString = milli;

    return minsString + ":" + secsString + "." + millString;
}

/**
 * DateToString
 * @param uint64 dateuint64
 * @return cString
 */
cString DateToString( uint64 dateuint64 )
{
    // convert dates to printable form
    cTime date = cTime(dateuint64);
    cString daysString, monsString, yearsString, hoursString, minsString, secsString;

    if ( date.min == 0 )
        minsString = "00";
    else if ( date.min < 10 )
        minsString = "0" + date.min;
    else
        minsString = date.min;

    if ( date.sec == 0 )
        secsString = "00";
    else if ( date.sec < 10 )
        secsString = "0" + date.sec;
    else
        secsString = date.sec;

    if ( date.hour == 0 )
        hoursString = "00";
    else if ( date.hour < 10 )
        hoursString = "0" + date.hour;
    else
        hoursString = date.hour;

    if ( date.mon == 0 )
        monsString = "00";
    else if ( date.mon < 10 )
        monsString = "0" + date.mon;
    else
        monsString = date.mon;

    if ( date.mday == 0 )
        daysString = "00";
    else if ( date.mday < 10 )
        daysString = "0" + date.mday;
    else
        daysString = date.mday;

    return daysString + "/" + monsString + "/" + (1900+date.year) + " " + hoursString +":" + minsString + ":" + secsString;
}

/**
 * Racesow_GetPlayerByClient
 * @param cClient @client
 * @return Racesow_Player
 */
Racesow_Player @Racesow_GetPlayerByClient( cClient @client )
{
    if ( @client == null || client.playerNum() < 0 )
        return null;

    return @racesowGametype.players[ client.playerNum() ].setClient( @client ); //FIXME: why set the client HERE??
}

/**
 * Racesow_GetPlayerByNumber
 * @param int playerNum
 * @return Racesow_Player
 */
Racesow_Player @Racesow_GetPlayerByNumber(int playerNum)
{
    if ( playerNum < 0 )
        return null;

    return @racesowGametype.players[ playerNum ];
}


/**
 * Racesow_GetClientNumber
 * @param cString playerName
 * @return int
 */
int Racesow_GetClientNumber( cString playerName )
{
    cClient @client;

    for ( int i = 0; i < maxClients; i++ )
    {
        @client = @G_GetClient( i );
        if ( client.state() < CS_SPAWNED )
            continue;

        if (client.getName().removeColorTokens() == playerName)
            return client.playerNum();
    }
    return -1;
}

//Jerm's Begin
/**
 * Racesow_GetClientByString
 * @param cString playerString
 * @return cClient
 */
cClient @Racesow_GetClientByString( cString playerString )
{

  cClient @client;
  
  if ( playerString.isNumerical() && playerString.toInt() < maxClients ) // !!!!!!!! cString.isNumerical() returns true for empty strings (wsw0.62) !!!!!!!!
  {
    @client = @G_GetClient( playerString.toInt() );
    
    if ( @client.getEnt() != null )
    {
      if ( client.getEnt().inuse )
        return @client;
    }
  }
  else
  {
    for ( int i = 0; i < maxClients; i++ )
    {
      @client = @G_GetClient( i );
      if ( client.getName().removeColorTokens() == playerString )
        return @client;
    }
  }
  return null;
}
//Jerm's End

/**
 * Cancel the current vote (equals to /opcall cancelvote)
 */
void RS_cancelvote()
{
    G_CmdExecute( "cancelvote" );
}

/**
 * Capitalize a string
 *
 * @param cString string
 * @return cString
 */
cString Capitalize( cString string )
{
    return string.substr(0,1).toupper() + string.substr(1,string.len()-1);
}

void RS_ircSendMessage( cString message )
{
    if( ircConnected == 0 )
        return;
    G_CmdExecute( "irc_chanmsg \"" + message + "\" \n");
}

/**
 * Find a modflag value by the gametype name - FIXME soon obsolete
 *
 * @param name The name of the gametype you are looking for
 * @return int the modflag value, -1 if not found
 */
int RS_GetModFlagByName(cString name)
{
    if ( name == "race" )
        return MODFLAG_RACE;
    if ( name == "freestyle" )
        return MODFLAG_FREESTYLE;
    if ( name == "fastcap" )
        return MODFLAG_FASTCAP;
    if ( name == "drace" )
        return MODFLAG_DRACE;
    if ( name == "durace" )
        return MODFLAG_DURACE;
    if ( name == "trace" )
        return MODFLAG_TRACE;

    G_Print("Gametype " + name + " doesn't exist. Check your config.\n");
    return -1;
}

/*
 * Insert Default Racesow_Commands into the gametypes Command_Map
 *
 */
void insertDefaultCommands( RC_Map @commandMap ) {
    /*@commandMap["admin"] = @Command_Admin();
    @commandMap["Command_AmmoSwitch"] = @Command_AmmoSwitch();
    @commandMap["auth"] = @Command_Auth();
    @commandMap["gametype"] = @Command_Gametype();
    @commandMap["help"] = @Command_Help();
    @commandMap["lastmap"] = @Command_LastMap();
    @commandMap["mapfilter"] = @Command_Mapfilter();
    @commandMap["maplist"] = @Command_Maplist();
    if (dedicated.getBool())
    {
        @commandMap["mapname"] = @Command_Mapname();
        @commandMap["nextmap"] = @Command_NextMap();
    }
    @commandMap["privsay"] = @Command_Privsay();
    @commandMap["protectednick"] = @Command_ProtectedNick();
    @commandMap["register"] = @Command_Register();
    @commandMap["stats"] = @Command_Stats();
    @commandMap["token"] = @Command_Token();
    @commandMap["whoisgod"] = @Command_WhoIsGod();
    @commandMap["callvotecheckpermission"] = @Command_CallvoteCheckPermission();
    @commandMap["callvotevalidate"] = @Command_CallvoteValidate();*/
    commandMap.set_opIndex( "admin", @Command_Admin() );
    commandMap.set_opIndex( "auth", @Command_Auth() );
    commandMap.set_opIndex( "gametype", @Command_Gametype() );
    commandMap.set_opIndex( "help", @Command_Help( null, @commandMap ) );
    commandMap.set_opIndex( "whoisgod", @Command_WhoIsGod() );
    commandMap.set_opIndex( "stats", @Command_Stats() );
}

/*
 * remove first token from a string
 * expects the string to be trimmed (no leading or trailing whitespaces)
 */
cString shiftArguments( cString args )
{
    if( args.length() < 2 )
        return "";

    //args = args.trim(); // it's already trimmed

    int pos;
    cString arg = args.getToken( 0 );
    if( arg == "" )
    { // we have to shift out an empty string. we assume empty string looks like this: ""
        for( pos = 0; pos < args.length(); pos++ )
        {
            if( args.subString( pos, 1 ) == "\"" )
            {
                pos += 2;
                break;
            }
        }
    }
    else
    {
        bool quote = false;
        for( pos = 0; pos < args.length(); pos++ )
        {
            if( args.subString( pos, 1 ) == "\"" )
                quote = !quote;
            if( args.subString( pos, arg.length() ) == arg )
            {
                if( quote )
                    pos += arg.length() + 1;
                else
                    pos += arg.length();
                break;
            }
        }
    }
    return args.subString( pos, args.length() - pos ).trim();
}
