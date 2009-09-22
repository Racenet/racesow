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
	 * Number of failed auths in a row
	 * @var uint
	 */
	uint failCount;
	
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
		
		if ( g_secureAuth.getBool() )
		{
			password = G_Md5( password );
		}
		
		// "authenticationName" "email" "password" "authorizationsMask" "currentTimestamp"
		G_WriteFile( authFile, '"'+ authName + '" "' + authEmail + '" "' + password + '" "' + 1 + '" "' + localTime + '"\n' );
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
				this.player.sendMessage( S_COLOR_RED + "usage: auth <account name> <password>\n" );
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
		
		if ( g_secureAuth.getBool() )
		{
			authPass = G_Md5( authPass );
		}
		
		if ( authPass != authContent.getToken( 2 ) )
		{
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
	
		this.failCount = 0;
		this.authenticationName = authName;
		this.authorizationsMask = uint( authContent.getToken( 3 ).toInt() );
		
		G_PrintMsg( null, S_COLOR_WHITE + this.player.getName() + S_COLOR_GREEN
			+ " successfully authenticated as "+ authName +"\n" );
		
		return true;
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
	 * @return bool
	 */
	bool wontGiveUpViolatingNickProtection()
	{
		if ( this.violateNickProtectionSince == 0 )
		{
			return false;
		}
		
		uint64 seconds = localTime - this.violateNickProtectionSince;
		
		this.player.getClient().addAward( "" + (10 - seconds) );
		
		return (seconds > 10);
	}
	
	/**
	 * Check if the palyer uses a protected nickname
	 * @return void
	 */
	void checkProtectedNickname()
	{
		cString name = this.player.getName().removeColorTokens();
		cString authFile = gameDataDir + "/nicknames/" + name.substr(0,1) + "/" + name;
		if ( G_LoadFile( authFile ) != this.authenticationName )
		{
			this.violateNickProtectionSince = localTime;
			this.player.getClient().addAward(S_COLOR_RED + "NICKNAME PROTECTION!");
			this.player.getClient().addAward(S_COLOR_RED + "CHECK THE CONSOLE NOW!");
			this.player.sendMessage( S_COLOR_RED + "You are using a protected nickname which dos not belong to you.\n"
				+ "If you don't authenticate or change your nickname within X TIMEUNIT you will be kicked.\n" );
		}
	}
 }