/**
 * Racesow_Map
 *
 * @package Racesow
 * @version 0.5.7
 */
class Racesow_Map
{
	cString name;

	bool inOvertime;
    
    bool overtimeFinished;

    uint id;
	
	bool firstAnnouncement;
	bool secondAnnouncement;
	bool thirdAnnouncement;

    /**
	 * Logfile id
	 * @var uint
	 */
	uint64 logTime;
    
	/*
	 * Highscore
	 */
    Racesow_Map_HighScore highScore;
	
	/*
	 * World record time for this map (loaded from database)
	 */
	uint worldBest;
    
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
		cVar mapName( "mapname", "", 0 );
        this.id = 0;
		this.name = mapName.getString();
		this.inOvertime = false;
        this.overtimeFinished = false;
        this.firstAnnouncement = false;
		this.secondAnnouncement = false;
		this.thirdAnnouncement = false;

        this.logTime = 0;
        this.highScore.reset();
	}
	
	void PrintMinutesLeft()
	{
		if ( ( match.duration() - levelTime ) < 180000 )
		{
			if ( !this.firstAnnouncement && match.duration() >= 180000 )
			{
				this.firstAnnouncement = true;
				G_PrintMsg( null, S_COLOR_GREEN + "3 minutes left...\n");
			}
		}
		else
		{
			this.firstAnnouncement = false;
		}

		if ( ( match.duration() - levelTime ) < 120000 )
		{
			if ( !this.secondAnnouncement && match.duration() >= 120000 )
			{
				this.secondAnnouncement = true;
				G_PrintMsg( null, S_COLOR_YELLOW + "2 minutes left...\n");
			}
		}
		else
		{
			this.secondAnnouncement = false;
		}

		if ( ( match.duration() - levelTime ) < 60000 )
		{
			if ( !this.thirdAnnouncement && match.duration() >= 60000 )
			{
				this.thirdAnnouncement = true;
				G_PrintMsg( null, S_COLOR_RED + "1 minutes left...\n");
			}
		}
		else
		{
			this.thirdAnnouncement = false;
		}
	}

    /**
     * Callback from game lib when a map was loaded
     * @return uint
     */
    void loadCallback(int id, int bestTime)
    {
        this.setId( id );
        if ( rs_loadHighscores.getBool() )
        {
            this.worldBest = bestTime; // save it for AS
        }
    }
    
    /**
	 * Get the map highscore
	 * @return Racesow_Map_HighScore
	 */
	Racesow_Map_HighScore @getHighScore()
	{
		return @this.highScore;
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
                 
                if ( players[i].getSpeed() == 0 )
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
					players[i].remove("You have been moved to spectators because you were idle during overtime.");
                    players[i].cancelRace();
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
