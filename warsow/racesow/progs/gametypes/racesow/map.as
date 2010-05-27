/**
 * Racesow_Map
 *
 * @package Racesow
 * @version 0.5.2
 */
class Racesow_Map
{
	cString name;

	Racesow_Map_HighScore_Default @statsHandler;
	
    uint nextCountdownCheck;
    
	bool inOvertime;
	
	/**
	 * Constructor
	 *
	 */
	Racesow_Map()
	{
		this.reset();
	}
	
	/**
	 * Destructor
	 *
	 */
	~Racesow_Map()
	{
	}

	void reset()
	{
		@this.statsHandler = Racesow_Map_HighScore_Default();
		this.statsHandler.setMap(@this);
	
		cVar mapName( "mapname", "", 0 );
		this.name = mapName.getString();
		this.inOvertime = false;
	}
	
    /**
	 * Show the overtime countdown to the players
	 * @return void
	 */
    void showCountDown()
    {
        if (levelTime < this.nextCountdownCheck)
            return;
        
        this.nextCountdownCheck = levelTime + 1000;
        
        uint secondsLeft = (g_timelimit.getInteger() * 60000 - levelTime) / 1000 + 10;
        switch( secondsLeft )
        {
            case 180:
            case 120:
            case 60:
                for ( int i = 0; i < maxClients; i++ )
    		    {
                    if (@players[i].client != null)
                    {
                        players[i].client.addAward(S_COLOR_YELLOW + secondsLeft / 60 + " minute" + (secondsLeft != 60 ? "s" : "") + " left");
                    }
    			}
        }
    }   
    
	/**
	 * allowEndGame
	 * @return bool
	 */
	bool allowEndGame()
	{
		if (!this.inOvertime)
		{
			uint numRacing = 0;
			
			for ( int i = 0; i < maxClients; i++ )
		    {
				if ( players[i].isRacing() )
				{
					players[i].startOvertime();
					numRacing++;
				}
			}
			
			if ( numRacing != 0 )
			{
				this.inOvertime = true;
				G_AnnouncerSound( null, G_SoundIndex( "sounds/announcer/overtime/overtime.ogg"), GS_MAX_TEAMS, false, null );
			}
		}
		else
		{
			uint numInOvertime = 0;
		
			for ( int i = 0; i < maxClients; i++ )
		    {
				if ( players[i].inOvertime )
				{
					cVec3 velocity = players[i].getClient().getEnt().getVelocity();
					
					if ( velocity.x == 0 && velocity.y == 0 && velocity.z == 0 )
					{
						if ( !players[i].startedIdling() )
						{
							players[i].startIdling();
						}
					}
					else if ( players[i].startedIdling() )
					{
						players[i].stopIdling();
					}
					
					if ( players[i].startedIdling() && players[i].getIdleTime()  > 5000 )
					{
						players[i].cancelRace();
						G_PrintMsg( players[i].getClient().getEnt(), S_COLOR_RED
							+ "You have been moved to spectators because you were idle during overtime.\n" );
					}
					else
					{
						numInOvertime++;
					}
				}
			}
			
			if ( numInOvertime == 0 )
			{
				this.inOvertime = false;
			}
		}
		
		return !this.inOvertime;
	}
	
	Racesow_Map_HighScore_Abstract @getStatsHandler()
	{
		return @this.statsHandler;
	}
	
	/**
	 * setUpMatch
	 * @return void
	 */
	void setUpMatch()
	{
	    int i, j;
	    cEntity @ent;
	    cTeam @team;

	    gametype.shootingDisabled = false;
	    gametype.readyAnnouncementEnabled = false;
	    gametype.scoreAnnouncementEnabled = false;
	    gametype.countdownEnabled = true;

	    gametype.pickableItemsMask = gametype.spawnableItemsMask;
	    gametype.dropableItemsMask = gametype.spawnableItemsMask;

	    // clear player stats and scores, team scores

	    for ( i = TEAM_PLAYERS; i < GS_MAX_TEAMS; i++ )
	    {
	        @team = @G_GetTeam( i );
	        team.stats.clear();
	    }

	    G_RemoveDeadBodies();
	}
}