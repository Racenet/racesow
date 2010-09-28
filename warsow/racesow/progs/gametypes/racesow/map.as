/**
 * Racesow_Map
 *
 * @package Racesow
 * @version 0.5.5
 */
class Racesow_Map
{
	cString name;

	Racesow_Map_HighScore_Abstract @statsHandler;

	bool inOvertime;
    
    bool overtimeFinished;

    uint id;

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

    /**
     * get the map's ID
     * @return uint
     */
    uint getId()
    {
        return this.id;
    }

    /**
     * get the map's ID
     * @return uint
     */
    void setId(uint id)
    {
        this.id = id;
    }

	void reset()
	{
	    if ( rs_mysqlEnabled.getBool() )
	        @this.statsHandler = Racesow_Map_HighScore_Mysql();
	    else
            @this.statsHandler = Racesow_Map_HighScore_NoMysql();

		this.statsHandler.setMap(@this);

		cVar mapName( "mapname", "", 0 );
        this.id = 0;
		this.name = mapName.getString();
		this.inOvertime = false;
        this.overtimeFinished = false;
	}

    void startOvertime()
    {
        if (this.inOvertime)
		{
            return;
        }
        
        uint numRacing = 0;

        for ( int i = 0; i < maxClients; i++ )
        {
            if ( @players[i] != null &&
                 @players[i].getClient() != null &&
                 players[i].isRacing() )
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
    
	/**
	 * allowEndGame
	 * @return bool
	 */
	bool allowEndGame()
	{
        if (!this.inOvertime)
        {
            return true;
        }
        else if (this.overtimeFinished)
        {
            return true;
        }
    
		uint numInOvertime = 0;
        for ( int i = 0; i < maxClients; i++ )
        {
            if (@players[i] != null &&
                @players[i].getClient() != null &&
                players[i].getClient().team != TEAM_SPECTATOR &&
                players[i].inOvertime) {
                 
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
            this.overtimeFinished = true;
            match.launchState(MATCH_STATE_POSTMATCH);
            return true;
        }
        
		return false;
	}

	/**
	 * cancelOvertime
	 * @return void
	 */

	void cancelOvertime()
	{
		this.inOvertime = false;
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
