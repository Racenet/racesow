/**
 * Racesow_Player_Auth
 *
 * @package Racesow
 * @subpackage Player
 * @version 0.5.3
 */

 class Racesow_Player_Auth : Racesow_Player_Implemented
 {
	/**
	 * Session name
	 * @var cString
	 */
	cString sessionName;

	/**
	 * Racesow account name
	 * @var cString
	 */
	cString authenticationName;
    
	/**
	 * The Player's unique ID
	 * @var int
	 */
	int playerId;
	
	/**
	 * The Player's nick unique ID
	 * @var int
	 */
	int nickId;
    
    /**
	 * Racesow authorizations bitmask
	 * @var uint
	 */
	uint authorizationsMask;

	/**
	 * The last auth name found in userinfo
	 * @var cString
	 */
	cString lastRefreshName;

	/**
	 * The last auth pass found in userinfo
	 * @var cString
	 */
	cString lastRefreshPass;

	/**
	 * Number of failed auths in a row
	 * @var uint
	 */
	uint failCount;

	/**
	 * The time of the last message
	 * @var int
	 */
	int lastViolateProtectionMessage;

	/**
	 * the time when we started to wait for the player
	 * to authenticate as he uses a protected nickname
	 * @var uint64
	 */
	uint64 violateNickProtectionSince;

	Racesow_Player_Auth()
	{
		this.reset();
	}

	void reset()
	{
		this.authenticationName = "";
		this.authorizationsMask = 0;
		this.failCount = 0;
		this.playerId = 0;
	}

	~Racesow_Player_Auth()
	{
	}

    /**
	 * Convert a string to an allowed filename
	 */
	cString toFileName(cString fileName)
	{
		cString outName="";
		int position = 0;
		for (position = 0; position < fileName.len(); position++)
		{
			cString character;
			character=fileName.substr(position,1);
			if (character=='|' || character=='<' || character=='>' || character=='?' ||  character=='!' || character=='\\' || character=='/' || character=='%' || character==':' || character=='*')
				outName+="_";
			else
				outName+=fileName.substr(position,1);
		}
		return outName;
	}
    
	/**
	 * Register a new server account
	 *
	 * You can login using either your authName, your email or any
	 * of your nicknames plus your password. This requires to avoid
	 * cross-type duplicates, means you cannot for example register
	 * an auth-name which has already been taken as nickname or email.
	 *
	 * @param cString &authName
	 * @param cString &authEmail
	 * @param cString &password
	 * @param cString &confirmation
	 * @return bool
	 */
	bool signUp(cString &authName, cString &authEmail, cString &password, cString &confirmation)
	{
		this.player.sendMessage( S_COLOR_RED + "Registration is currently not available\n" );
		return false;
	}

	/**
	 * Authenticate server account
	 * @param cString &authName
	 * @param cString &password
	 * @return bool
	 */
	bool authenticate( cString &authName, cString &authPass, bool autoAuth )
	{
		if ( authName == "" || authPass == "" )
		{
			if ( !autoAuth )
			{
				this.player.sendMessage( S_COLOR_RED + "usage: auth <account name> <password>\n" );
			}

			return false;
		}

		cString PlayerNick = this.player.getName().removeColorTokens();
		
        this.authenticationName = authName;
		// passing playerNum(), but there's a very small risk that the same playerNum switches to another player if he disconnects before auth finishes..
        RS_MysqlAuthenticate(this.player.getClient().playerNum(), authName, authPass); 
        return true;
	}
    
    /**
	 * Callback for the authentication gets called from the game lib's worker thread
     *
     * @param int playerId
	 * @param int authMask
     * @return void
	 */
    void authCallback( int playerId, int authMask )
    {
        if (playerId == 0) {
        
            G_PrintMsg( null, S_COLOR_WHITE + this.player.getName() + S_COLOR_RED + " failed to authenticate as "+ this.authenticationName +"\n" );
            this.authenticationName = "";
            return;
        }
        
        if ( this.lastViolateProtectionMessage != 0 )
		{
            this.player.sendMessage( S_COLOR_GREEN + "Countdown stopped.\n" );
		}

        this.playerId = playerId;
		this.failCount = 0;
		this.violateNickProtectionSince = 0;
		this.lastViolateProtectionMessage = 0;
		this.authorizationsMask = uint(authMask);
        
		G_PrintMsg( null, S_COLOR_WHITE + this.player.getName() + S_COLOR_GREEN + " successfully authenticated as "+ this.authenticationName +"\n" );
	}

    /**
	 * Callback for the nickname protection
     *
     * @param int playerId
     * @return void
	 */
    void nickProtectCallback( int wanted_playerId, int current_playerId )
    {
        if ( ( wanted_playerId == current_playerId && this.isAuthenticated() ) || ( wanted_playerId == 0 ))
        {
            if ( this.lastViolateProtectionMessage != 0 )
    		{
                this.violateNickProtectionSince = 0;
                this.lastViolateProtectionMessage = 0;
                this.player.sendMessage( S_COLOR_GREEN + "Countdown stopped.\n" );
    		}
        }
        else if ( wanted_playerId != 0 )
		{
			if ( this.violateNickProtectionSince == 0 )
			{
				this.violateNickProtectionSince = localTime;
				this.player.sendMessage( S_COLOR_RED + "NICKNAME PROTECTION!\n");
				this.player.sendMessage( S_COLOR_RED + "You are using a protected nickname which does not belong to you.\n"
				+ "If you don't authenticate or change your nickname you will be kicked.\n" );
			}

		}
    }

	/**
	 * isAuthenticated
	 * @return bool
	 */
	bool isAuthenticated()
	{
		return this.authorizationsMask > 0;
	}

	/**
	 * Check for a nickname change event
	 * @return void
	 */
	void refresh( cString &nick)
	{
		if ( @this.player == null || @this.player.getClient() == null )
			return;

		if ( nick.removeColorTokens() != this.player.getName().removeColorTokens() )
		{
			RS_MysqlPlayerDisappear(nick, levelTime-this.player.joinedTime, this.player.getId(), this.player.getNickId() , map.getId(), this.player.getAuth().isAuthenticated());
			player.joinedTime = levelTime;
			RS_MysqlPlayerAppear(this.player.getName(), this.player.getClient().playerNum(), this.player.getId(), map.getId(), this.player.getAuth().isAuthenticated());
		}
	}

	/**
	 * Check if the player is authorized to do something
	 * @param const uint permission
	 * @return bool
	 */
	bool allow( const uint permission )
	{
		return ( this.authorizationsMask & permission != 0 );
	}
    
	/**
	 * Get the player's status concerning nickname protection
	 * @return int
	 */
	int wontGiveUpViolatingNickProtection()
	{
		if ( this.violateNickProtectionSince == 0 )
		{
			return 0;
		}

		int seconds = localTime - this.violateNickProtectionSince;
		if ( seconds == this.lastViolateProtectionMessage )
			return -1; // nothing to do

		this.lastViolateProtectionMessage = seconds;

		if ( seconds < 21 )
			return 1;

		return 2;
	}

	cString getViolateCountDown()
	{
		cString color;
		int seconds = localTime - this.violateNickProtectionSince;
		if ( seconds > 6 )
			color = S_COLOR_RED;
		else if ( seconds > 3 )
			color = S_COLOR_YELLOW;
		else
			color = S_COLOR_GREEN;

		return color + (21 - (localTime - this.violateNickProtectionSince)) + " seconds remaining...";
	}
}