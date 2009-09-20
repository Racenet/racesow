/**
 * Racesow_Map
 *
 */
class Racesow_Map
{
	cString name;

	Racesow_Map_HighScore_Default[] highScores;
	
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
		cVar mapName( "mapname", "", 0 );
		this.name = mapName.getString();
		this.inOvertime = false;
		this.highScores.resize( MAX_RECORDS );
		for ( int i = 0; i < MAX_RECORDS; i++ )
		{
			this.highScores[i].reset();
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
							+ "You have been moved to spectators because you were idle during overtime." );
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
	
	/**
	 * handleCheckpoint
	 * @param Racesow_Player_Race @race,
	 * @param uint id
	 */
	void handleCheckpoint(Racesow_Player_Race @race, uint id)
	{
		Racesow_Player @player = race.getPlayer();
		uint newTime = race.getCheckPoint(id);
		uint serverBestTime = this.highScores[0].getCheckPoint(id);
		uint personalBestTime = player.getBestCheckPoint(id);
		
		// if beating the checkpoint record on this sector give an award
        if ( newTime < serverBestTime || serverBestTime == 0 )
        {
			if ( this.highScores[0].setCheckPoint(id, newTime) && player.setBestCheckPoint(id, newTime) )
			{
				race.triggerAward( S_COLOR_GREEN + (id + 1) + ". checkpoint record!" );
				G_Print(player.client.getName() + " " + S_COLOR_WHITE + "made a new "
					+ (id + 1) + ". checkpoint record: " + TimeToString( newTime ) + "\n" );
			}
        }
        // else if beating his own record on this secotr give an award
        else if ( newTime < personalBestTime || personalBestTime == 0 )
        {
            if ( player.setBestCheckPoint(id, newTime) )
				race.triggerAward( S_COLOR_YELLOW + (id + 1) + ". checkpoint personal record!" );
        }
	}
	
	/**
	 * handleFinishLine
	 * @param Racesow_Player_Race @race
	 * @return void
	 */
	void handleFinishedRace(Racesow_Player_Race @race)
	{
		uint newTime = race.getTime();
	
        // see if the player improved one of the top scores
		for ( int top = 0; top < MAX_RECORDS; top++ )
		{
			uint oldTime = this.highScores[top].getTime();
			if ( newTime < oldTime || oldTime == 0 )
			{
				// move the other records down
				for ( int i = MAX_RECORDS - 1; i > top; i-- )
				{
					if (this.highScores[i - 1].getTime() == 0)
						break;
						
					this.highScores[i] = this.highScores[i - 1];
				}

				this.highScores[top].fromRace( race );

				this.writeHighScores();
				this.updateHud();
				break;
			}
		}
	}
	
	/**
	 * WriteHighScores
	 *
	 */
	void writeHighScores()
	{
		cString highScores = "//" + this.name + " top scores\n\n";

	    for ( int i = 0; i < MAX_RECORDS; i++ )
	    {
	        int time = this.highScores[i].getTime();
			cString playerName = this.highScores[i].getPlayerName();
			
			if ( time > 0 && playerName.len() > 0 )
	        {
	            highScores += "\"" + time + "\" \"" + playerName + "\" ";

	            // add checkpoints
	            highScores += "\"" + numCheckpoints+ "\" ";

	            for ( int j = 0; j < numCheckpoints; j++ )
	                highScores += "\"" + int( this.highScores[i].getCheckPoint(j) ) + "\" ";

	            highScores += "\n";
	        }
	    }

	    G_WriteFile( "topscores/race/" + this.name + ".txt", highScores );
	}

	/**
	 * LoadHighScores
	 *
	 */
	void loadHighScores()
	{
	    cString highScores;

	    highScores = G_LoadFile( "topscores/race/" + this.name + ".txt" );

	    if ( highScores.len() > 0 )
	    {
	        cString timeToken, nameToken, sectorToken;
	        int count = 0;

	        for ( int i = 0; i < MAX_RECORDS; i++ )
	        {
	            timeToken = highScores.getToken( count++ );
	            if ( timeToken.len() == 0 )
	                break;

	            nameToken = highScores.getToken( count++ );
	            if ( nameToken.len() == 0 )
	                break;

	            sectorToken = highScores.getToken( count++ );
	            if ( sectorToken.len() == 0 )
	                break;

	            int numSectors = sectorToken.toInt();

	            // store this one
	            for ( int j = 0; j < numSectors; j++ )
	            {
	                sectorToken = highScores.getToken( count++ );
	                if ( sectorToken.len() == 0 )
	                    break;

	                this.highScores[i].checkPoints[j] = uint( sectorToken.toInt() );
	            }

	            this.highScores[i].finishTime = uint( timeToken.toInt() );
	            this.highScores[i].playerName = nameToken;
	        }

	        this.updateHud();
	    }
	}
	
	/**
	 * UpdateHud
	 * @return void
	 */
	void updateHud()
	{
	    for ( int i = 0; i < MAX_RECORDS; i++ )
	    {
	        if ( this.highScores[i].finishTime > 0 && this.highScores[i].playerName.len() > 0 )
	        {
	            G_ConfigString( CS_GENERAL + i, "#" + ( i + 1 ) + " - " + this.highScores[i].playerName + " - " + TimeToString( this.highScores[i].finishTime ) );
	        }
	    }
	}
	
	/**
	 * getHighscores
	 * @return cString
	 */
	cString getHighscores()
	{
		cString highScores = "";

	    for ( int i = 0; i < MAX_RECORDS; i++ )
	    {
	        int time = this.highScores[i].getTime();
			if ( time > 0 )
			{
				cString playerName = this.highScores[i].getPlayerName();
				highScores += (i+1) + ". " + playerName + " ("+ TimeToString(time) +")\n";
			}
	    }

	    return highScores;
	}
}