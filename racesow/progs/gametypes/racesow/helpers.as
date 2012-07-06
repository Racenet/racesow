/**
 * Helper functions
 */

/**
 * Print the Message when the Test fails and abort the Gametype by using a nullpointer access (dirty hack!)
 *
 * @param test The Test or Condition which will be used
 * @param msg The Message which is printed when the test fails
 */
void assert( const bool test, const String msg )
{
	if ( !test )
	{
        String@ assertstring;
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
String diffString( uint oldTime, uint newTime )
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
 * @return String
 */
String TimeToString( uint time )
{
    // convert times to printable form
    String minsString, secsString, millString;
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
 * @return String
 */
String DateToString( uint64 dateuint64 )
{
    // convert dates to printable form
    Time date = Time(dateuint64);
    String daysString, monsString, yearsString, hoursString, minsString, secsString;

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
 * @param String playerName
 * @return int
 */
int Racesow_GetClientNumber( String playerName )
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
 * @param String playerString
 * @return cClient
 */
cClient @Racesow_GetClientByString( String playerString )
{

  cClient @client;
  
  if ( playerString.isNumerical() && playerString.toInt() < maxClients ) // !!!!!!!! String.isNumerical() returns true for empty strings (wsw0.62) !!!!!!!!
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
 * @param String string
 * @return String
 */
String Capitalize( String string )
{
    return string.substr(0,1).toupper() + string.substr(1,string.len()-1);
}

void RS_ircSendMessage( String message )
{
    if( ircConnected == 0 )
        return;
    G_CmdExecute( "irc_chanmsg \"" + message + "\" \n");
}

/*
 * Insert Default Racesow_Commands into the gametypes Command_Map
 *
 */
void insertDefaultCommands( RC_Map @commandMap ) {
    @commandMap["admin"] = @Command_Admin();
    @commandMap["ammoswitch"] = @Command_AmmoSwitch();
    @commandMap["auth"] = @Command_Auth();
    @commandMap["gametype"] = @Command_Gametype();
    @commandMap["help"] = @Command_Help( null, @commandMap );
    @commandMap["stats"] = @Command_Stats();
//    @commandMap["token"] = @Command_Token();
    @commandMap["whoisgod"] = @Command_WhoIsGod();
}

/*
 * remove first token from a string
 * expects the string to be trimmed (no leading or trailing whitespaces)
 */
String shiftArguments( String args )
{
    if( args.length() < 2 )
        return "";

    //args = args.trim(); // it's already trimmed

    uint pos;
    String arg = args.getToken( 0 );
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
