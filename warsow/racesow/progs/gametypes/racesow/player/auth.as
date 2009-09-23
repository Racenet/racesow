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
	 * @var uint64
	 */
	uint64 lastViolateProtectionMessage;
	
	/**
	 * the time when we started to wait for the player
	 * to authenticate as he uses a protected nickname
	 * @var uint64
	 */
	uint64 violateNickProtectionSince;
	
	Racesow_Player_Auth()
	{
		this.authenticationName = "";
		this.authorizationsMask = 0;
		this.failCount = 0;
	}
	
	~Racesow_Player_Auth()
	{
		this.killSession();
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
		cString nickName = this.player.getName().removeColorTokens();
	
		cString authFile = gameDataDir + "/auths/" + authName.substr(0,1) + "/" + authName;
		cString mailShadow = gameDataDir + "/emails/" + authEmail.substr(0,1) + "/" + authEmail;
		cString nickShadow = gameDataDir + "/nicknames/" + nickName.substr(0,1) + "/" + nickName;
		
		cString duplicateNameCheckNick = gameDataDir + "/auths/" + nickName.substr(0,1) + "/" + nickName;
		cString duplicateNameCheckEmail = gameDataDir + "/auths/" + authEmail.substr(0,1) + "/" + authEmail;
		cString duplicateEmailCheckName = gameDataDir + "/emails/" + authName.substr(0,1) + "/" + authName;
		cString duplicateEmailCheckNick = gameDataDir + "/emails/" + nickName.substr(0,1) + "/" + nickName;
		cString duplicateNickCheckEmail = gameDataDir + "/nicknames/" + authEmail.substr(0,1) + "/" + authEmail;
		cString duplicateNickCheckName = gameDataDir + "/nicknames/" + authName.substr(0,1) + "/" + authName;
		
		if ( authName == "" || authEmail == "" || password == "" || confirmation == "" )
		{
			this.player.sendMessage( S_COLOR_RED + "usage: register <account name> <account email> <password> <confirm password>\n" );
			return false;
		}
		
		if ( password != confirmation )
		{
			this.player.sendMessage( S_COLOR_RED + "Error: passwords do not match\n" );
			return false;
		}
		
	    if ( G_FileLength( authFile ) != -1 ||
			 G_FileLength( duplicateNameCheckNick ) != -1 ||
			 G_FileLength( duplicateNameCheckEmail ) != -1 )
		{
			this.player.sendMessage( S_COLOR_RED + "Error: Your login '" + authName + "' is already registered.\n" );
			return false;
		}
		
		if ( G_FileLength( mailShadow ) != -1 ||
			G_FileLength( duplicateEmailCheckName ) != -1 ||
			G_FileLength( duplicateEmailCheckNick ) != -1 )
		{
			this.player.sendMessage( S_COLOR_RED + "Error: Your email '" + authEmail + "' is already registered.\n" );
			return false;
		}		
		
		if ( G_FileLength( nickShadow ) != -1 ||
			G_FileLength( duplicateNickCheckName ) != -1 ||
			G_FileLength( duplicateNickCheckEmail ) != -1 )
		{
			this.player.sendMessage( "Your nickname '" + this.player.getName() + "' is already registered.\n" );
			return false;
		}
		
		// TODO: check email for valid format
		
		// "authenticationName" "email" "password" "authorizationsMask" "currentTimestamp"
		G_WriteFile( authFile, '"'+ authName + '" "' + authEmail + '" "' + G_Md5( password ) + '" "' + 1 + '" "' + localTime + '"\n' );
		G_WriteFile( mailShadow, authName );
		G_WriteFile( nickShadow, authName );
		
		this.player.sendMessage( S_COLOR_GREEN + "Successfully registered as "
			+ authName + ". " + S_COLOR_WHITE + "Don't forget your password...\n" );
		
		return true;
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
		
		cString authFile = gameDataDir + "/auths/" + authName.substr(0,1) + "/" + authName;
		
		if ( G_FileLength( authFile ) == -1 )
		{
			authFile = gameDataDir + "/emails/" + authName.substr(0,1) + "/" + authName;
			if ( G_FileLength( authFile ) == -1 )
			{
				authFile = gameDataDir + "/nicknames/" + authName.substr(0,1) + "/" + authName;
				if ( G_FileLength( authFile ) != -1 )
				{
					authName = G_LoadFile( authFile );
					authFile = gameDataDir + "/auths/" + authName.substr(0,1) + "/" + authName;
				}
			}
			else
			{
				authName =  G_LoadFile( authFile );
				authFile = gameDataDir + "/auths/" + authName.substr(0,1) + "/" + authName;
			}
			
			if ( G_FileLength( authFile ) == -1 )
			{
				this.player.sendMessage( S_COLOR_RED + "Error: "+ authName +" is not registered\n" );
				return false;
			}
		}

		cString authContent = G_LoadFile( authFile );
		if ( G_Md5( authPass ) != authContent.getToken( 2 ) )
		{
			if ( this.authorizationsMask > 0 )
			{
				this.player.sendMessage(S_COLOR_RED + "Your authentication info changed but is invalid now.");
				return false;
			}
			
			this.failCount++;
			
			if ( this.failCount >= 3 ) 
			{
				// TODO: kick player with reason instead of this print
				G_PrintMsg( null, S_COLOR_WHITE + this.player.getName() + S_COLOR_RED
					+ " kicked due to too many failed logins. (TODO)\n" );
			}
			else 
			{
				G_PrintMsg( null, S_COLOR_WHITE + this.player.getName() + S_COLOR_RED 
					+ " failed in authenticating as "+ authName +"\n" );
			}
			
			return false;
		}
	
		if ( this.lastViolateProtectionMessage != 0 )
		{
			this.player.getClient().addAward( S_COLOR_GREEN + "Countdown stopped." );
		}
	
		this.failCount = 0;
		this.violateNickProtectionSince = 0;
		this.lastViolateProtectionMessage = 0;
		this.authenticationName = authName;
		this.authorizationsMask = uint( authContent.getToken( 3 ).toInt() );
		
		this.writeSession();
		
		G_PrintMsg( null, S_COLOR_WHITE + this.player.getName() + S_COLOR_GREEN
			+ " successfully authenticated as "+ authName +"\n" );
		
		return true;
	}
	
	/**
	 * Refresh the authentication when it changed
	 * @return void
	 */
	void refresh( cString &args )
	{
		// when authentication information changed, try to authenticate again
		cString authName = this.player.getClient().getUserInfoKey("auth_name");
		cString authPass = this.player.getClient().getUserInfoKey("auth_pass");
		if ( authName != this.lastRefreshName && authPass != this.lastRefreshPass )
		{
			this.lastRefreshName = authName;
			this.lastRefreshPass = authPass;
			this.authenticate( authName, authPass, true );
		}
		
		if ( args.getToken(0).removeColorTokens() != this.player.getName().removeColorTokens() )
		{
			this.checkProtectedNickname();
		}
	}
	
	void writeSession()
	{
		this.sessionName = this.player.getName().removeColorTokens();
		cString sessionFile = gameDataDir + "/sessions/" + this.sessionName.substr(0,1) + "/" + this.sessionName;
		G_WriteFile( sessionFile, '"' + this.player.getClient().getUserInfoKey("ip") + '" "' + this.authenticationName + '" "' + this.authorizationsMask +'"\n' );
	}	
	
	bool loadSession()
	{
		this.sessionName = this.player.getName().removeColorTokens();
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
	 * Check if the user should finallly get kicked due to
	 * violation against the nickname protection and send
	 * countdown "awards"
	 * @return int
	 */
	int wontGiveUpViolatingNickProtection()
	{
		if ( this.violateNickProtectionSince == 0 )
		{
			return 0;
		}

		int seconds = localTime - this.violateNickProtectionSince;
		if ( seconds != 0 && seconds == this.lastViolateProtectionMessage )
			return -1; // nothing to do
			
		this.lastViolateProtectionMessage = seconds;
			
		if ( seconds < 10 )
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
	
		return color + (10 - (localTime - this.violateNickProtectionSince)) + " seconds remaining...";
	}
	
	/**
	 * Check if the palyer uses a protected nickname
	 * @return void
	 */
	void checkProtectedNickname()
	{
		cString name = this.player.getName().removeColorTokens();
		cString authFile = gameDataDir + "/nicknames/" + name.substr(0,1) + "/" + name;
		
		if ( G_FileLength( authFile ) != -1 && G_LoadFile( authFile ) != this.authenticationName )
		{
			this.violateNickProtectionSince = localTime;
			this.player.getClient().addAward(S_COLOR_RED + "NICKNAME PROTECTION!");
			this.player.getClient().addAward(S_COLOR_RED + "CHECK THE CONSOLE NOW!");
			this.player.sendMessage( S_COLOR_RED + "You are using a protected nickname which dos not belong to you.\n"
				+ "If you don't authenticate or change your nickname within X TIMEUNIT you will be kicked.\n" );
		
		}
		// if authenticated, add new nickShadow
		else if ( this.authorizationsMask > 0 )
		{
			cString nickName = this.player.getName().removeColorTokens();
			cString nickShadow = gameDataDir + "/nicknames/" + nickName.substr(0,1) + "/" + nickName;
			G_WriteFile( nickShadow, this.authenticationName );
		}
	}
 }