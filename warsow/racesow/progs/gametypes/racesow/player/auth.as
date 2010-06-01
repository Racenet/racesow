/**
 * Racesow_Player_Auth
 *
 * @package Racesow
 * @subpackage Player
 * @version 0.5.1c
 * @date 23.09.2009
 * @author soh-zolex <zolex@warsow-race.net>
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
    
    void nickProtectCallback( bool hasValidNickname )
    {
        if ( !hasValidNickname )
		{
			this.violateNickProtectionSince = localTime;
            this.player.sendMessage( S_COLOR_RED + "NICKNAME PROTECTION!\n");
			this.player.sendMessage( S_COLOR_RED + "You are using a protected nickname which does not belong to you.\n"
				+ "If you don't authenticate or change your nickname you will be kicked.\n" );

		}
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
		this.writeSession();
        
		G_PrintMsg( null, S_COLOR_WHITE + this.player.getName() + S_COLOR_GREEN + " successfully authenticated as "+ this.authenticationName +"\n" );
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
	 * Refresh the authentication when it changed
	 * @return void
	 */
	void refresh( cString &nick )
	{
		// when authentication information changed, try to authenticate again
		if ( @this.player == null || @this.player.getClient() == null )
			return;

        /*
		cString authName = this.player.getClient().getUserInfoKey("auth_name");
		cString authPass = this.player.getClient().getUserInfoKey("auth_pass");
		if ( authName != this.lastRefreshName && authPass != this.lastRefreshPass )
		{
            this.player.sendMessage("Authenticate: " + authName + ", " + authPass + "\n");
        
			this.lastRefreshName = authName;
			this.lastRefreshPass = authPass;
			this.authenticate( authName, authPass, true );
		}
        */
        
		if ( nick.getToken(0).removeColorTokens() != this.player.getName().removeColorTokens() )
		{
			this.checkProtectedNickname();
		}
	}

	void writeSession()
	{
		this.sessionName = toFileName(this.player.getName().removeColorTokens());
		cString sessionFile = gameDataDir + "/sessions/" + this.sessionName.substr(0,1) + "/" + this.sessionName;
		G_WriteFile( sessionFile, '"' + this.player.getClient().getUserInfoKey("ip") + '" "' + this.authenticationName + '" "' + this.authorizationsMask +'"\n' );
	}

	bool loadSession()
	{
		this.sessionName = toFileName(this.player.getName().removeColorTokens());
		cString sessionFile = gameDataDir + "/sessions/" + this.sessionName.substr(0,1) + "/" + this.sessionName;
		cString sessionContent = G_LoadFile( sessionFile );

		if ( this.player.getClient().getUserInfoKey("ip") == sessionContent.getToken( 0 ) ) {

			this.failCount = 0;
			this.violateNickProtectionSince = 0;
			this.lastViolateProtectionMessage = 0;
			this.authenticationName = sessionContent.getToken( 1 );
			this.authorizationsMask = uint( sessionContent.getToken( 2 ).toInt() );

			G_PrintMsg( null, S_COLOR_WHITE + this.player.getName() + S_COLOR_GREEN
				+ " successfully loaded session for "+ this.authenticationName +"\n" );

			return true;
		}

		return false;
	}

	void killSession()
	{
		cString sessionFile = gameDataDir + "/sessions/" + this.sessionName.substr(0,1) + "/" + this.sessionName;
		G_WriteFile( sessionFile, "0" );
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

	/**
	 * Check if the player uses a protected nickname
	 * @return void
	 */
	void checkProtectedNickname()
	{
        // RS_MysqlNickProtection(this.player.getClient().getEnt());
	}
}


/**
 * Callback functions are called from the racesow game lib
 *
	 * @param int playerNum
     * @param int playerId
	 * @param int authMask
     * @return void

 */

void RS_MysqlAuthenticate_Callback( int playerNum, int playerId, int authMask )
{
    Racesow_GetPlayerByNumber(playerNum).getAuth().authCallback( playerId, authMask );
}

/*
void RS_MysqlNickprotection_Callback( cEntity @ent, bool result )
{
     if (@ent != null) {
    
        Racesow_GetPlayerByClient(ent.client).getAuth().nickProtectCallback( result );
    
    } else {
    
        G_PrintMsg(null, S_COLOR_RED + "error while checking nickname protection\n");
    }
}
/*

