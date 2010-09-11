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
	 * Racesow account name
	 * @var cString
	 */
	cString authenticationName;
    
    /**
	 * Racesow account pass
	 * @var cString
	 */
	cString authenticationPass; 
  
    
    /**
	 * Racesow account token
	 * @var cString
	 */
	cString authenticationToken;

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

    /**
     * Constructor
     */
	Racesow_Player_Auth()
	{
		this.reset();
	}

        
    /**
     * Descructor
     */
	~Racesow_Player_Auth()
	{
	}
    
    /**
     * Reset the whole auth object
     *
     */
	void reset()
	{
        this.killAuthentication();
        this.resetViolateState();
	}

    /**
     * Don't blame the player nomore
     *
     * @return void
     */
    void resetViolateState()
    {
        this.violateNickProtectionSince = 0;
        this.lastViolateProtectionMessage = 0;
        this.failCount = 0;
    }
    
    /**
     * Remove all information about the authentication
     * and the authorizations of the player
     *
     * @return void
     */
    void killAuthentication()
    {
        this.nickId = 0;
        this.playerId = 0;
        this.authenticationName = "";
        this.authenticationPass = "";
        this.authenticationToken = "";
        this.authorizationsMask = 0;
    }

    Racesow_Player_Auth @setName(cString name)
    {
        this.authenticationName = name;
        return @this;
    }
    
    Racesow_Player_Auth @setPass(cString pass)
    {
        this.authenticationPass = pass;
        return @this;
    }
    
    Racesow_Player_Auth @setToken(cString token)
    {
        this.authenticationToken = token;
        return @this;
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
        if (rs_registrationDisabled.getBool()) {
        
            this.player.sendMessage( S_COLOR_RED + rs_registrationInfo.getString() );
            return false;
        }
    
		// TODO:
        return true;
	}

    /**
	 * Show the login token to the player
     *
	 */
    bool showToken()
    {
        this.player.sendMessage( "The generation of tokens is not yet implemenmted. If you somehow got a working token you can ignore this message.\n" );
        return false;
    }
    
	/**
	 * Authenticate server account
	 * @param cString &authName
	 * @param cString &password
     * @param bool silent
	 * @return bool
	 */
	bool authenticate( cString &authName, cString &authPass, bool silent )
	{
		if ( authName == "" || authName == "" && authPass == "" )
		{
			if ( !silent )
			{
				this.player.sendMessage( S_COLOR_RED + "usage: auth <account name> <password> OR auth <token>\n" );
			}

			return false;
		}
        
        if (authName == this.authenticationName)
        {
            if ( !silent )
			{
				this.player.sendMessage( S_COLOR_RED + "You are already authed as " + authName + "\n" );
			}
            return false;
        }
        
        if (authName == this.authenticationToken)
        {
            if ( !silent )
			{
				this.player.sendMessage( S_COLOR_RED + "You are already authed with that token\n" );
			}
            return false;
        }
        
        RS_MysqlPlayerDisappear(
            this.player.getName(),
            levelTime-this.player.joinedTime,
            this.player.getId(),
            this.player.getNickId(),
            map.getId(),
            this.isAuthenticated()
        );
        
        // if only one param was passed, handle it as an authToken
        if (authPass == "")
        {
            this.setToken(authName);
            this.setName("");
            this.setPass("");
        }
        // otherwise it's username/password
        else
        {
            this.setToken("");
            this.setName(authName);
            this.setPass(authPass);
        }
        
        this.player.appear();
        
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
        if (playerId == 0)
        {
            if (this.authenticationToken != "")
            {
                G_PrintMsg( null, S_COLOR_WHITE + this.player.getName() + S_COLOR_RED + " failed to authenticate via token\n" );
            }
            else
            {
                G_PrintMsg( null, S_COLOR_WHITE + this.player.getName() + S_COLOR_RED + " failed to authenticate as "+ this.authenticationName +"\n" );
            }
            
            if (++this.failCount >= 3)
            {
                this.player.kick( "Multiple invalid login tries." );
            }
            
            this.killAuthentication();
        }
        else
        {
            this.authorizationsMask = uint(authMask);
            this.appearCallback(playerId);
            
            if (this.authenticationToken != "")
            {
                G_PrintMsg( null, S_COLOR_WHITE + this.player.getName() + S_COLOR_GREEN + " successfully authenticated via token\n" );
            }
            else
            {
                G_PrintMsg( null, S_COLOR_WHITE + this.player.getName() + S_COLOR_GREEN + " successfully authenticated as "+ this.authenticationName +"\n" );
            }
        }
	}

    /**
	 * Callback for the nickname protection
     *
     * @param int player_id_for_nick
     * @param int player_id
     * @return void
	 */
    void nickProtectCallback( int player_id_for_nick, int player_id )
    {
        if (player_id_for_nick != player_id && this.violateNickProtectionSince == 0)
        {
            this.violateNickProtectionSince = localTime;
			this.player.sendMessage( S_COLOR_RED + "NICKNAME PROTECTION!\n" + S_COLOR_RED + "If you don't authenticate or change your nickname you will be kicked.\n" );
			G_PrintMsg(null, this.player.getName() + S_COLOR_RED + " is using a protected nickname which does not belong to him.\n");
        }
    }
    
    /**
     * Callback when a player "appeared"
     *
     * @param int playerId
     * @return void
     */
    void appearCallback(int playerId)
    {
        this.playerId = playerId;
        this.player.sendMessage( S_COLOR_BLUE + "your playerId: "+ playerId +"\n" );
        
        if ( this.lastViolateProtectionMessage != 0 )
        {
            this.resetViolateState();
            this.player.sendMessage( S_COLOR_GREEN + "Countdown stopped.\n" );
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
	void refresh( cString &oldNick)
	{
		if ( @this.player == null || @this.player.getClient() == null )
			return;

		if ( oldNick.removeColorTokens() != this.player.getName().removeColorTokens() )
		{
			RS_MysqlPlayerDisappear(
                oldNick,
                levelTime-this.player.joinedTime,
                this.player.getId(),
                this.player.getNickId(),
                map.getId(),
                this.isAuthenticated()
            );
            
            this.player.appear();
        }
	}

	/**
	 * Check if the player is authorized to do something
	 * @param const uint permission
	 * @return bool
	 */
	bool allow( const uint permission )
	{
        this.player.sendMessage("mask: " + this.authorizationsMask + "\n");
    
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