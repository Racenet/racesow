/**
 * Racesow_Map_HighScore_Default
 *
 * Stores stats in a simple text file
 *
 * @package Racesow
 * @subpackage Map_HighScore
 * @version 0.5.1d
 * @date 24.09.2009
 * @author soh-zolex <zolex@warsow-race.net>
 */
class Racesow_Map_HighScore_Default : Racesow_Map_HighScore_Abstract
{
	/**
	 * Add a race to the highscores
	 * @param Racesow_Player_Race @race
	 * @return void
	 */
	void addRace(Racesow_Player_Race @race)
	{
		this.logRace(@race);
	
		// see if the player improved one of the top scores
		for ( int top = 0; top < MAX_RECORDS; top++ )
		{
			uint oldTime = this.highScores[top].getTime();
			if ( oldTime == 0 || race.getTime() < oldTime )
			{
				int skipPlayer = 0;
				// move the other records down
				for ( int i = MAX_RECORDS - 1; i > top; i-- )
				{
					if (this.highScores[i - 1].getTime() == 0)
						break;
					
					// if the same player has a worse time, do not keep it
					if (this.highScores[i-1].getPlayerName()==race.getPlayer().getClient().getName())
						skipPlayer = 1;

					this.highScores[i] = this.highScores[i - 1 - skipPlayer];
				}

				this.highScores[top].fromRace( race );

				this.writeStats();
				this.updateHud();
				break;
			}
			
			// if the same player already has a better time, don't do anything
			if (this.highScores[top].getPlayerName()==race.getPlayer().getClient().getName())
				break;
		}
	}
	
	void logRace(Racesow_Player_Race @race)
	{
		cVar g_logRaces( "g_logRaces", "0", 0 );
		if ( g_logRaces.getBool() )
			G_AppendToFile( "gamedata/races/" + this.map.name + "_" + this.logTime, race.toString() );
	}
	
	/**
	 * Write the stats to the file
	 * @return void
	 */
	void writeStats()
	{
		cString highScores = "//" + this.map.name + " top scores\n\n";

	    for ( int i = 0; i < MAX_RECORDS; i++ )
	    {
	        int time = this.highScores[i].getTime();
			cString playerName = this.highScores[i].getPlayerName();
			
			if ( time > 0 && playerName.len() > 0 )
	        {
	            highScores += "\"" + time + "\" \"" + playerName + "\" \"" + this.highScores[i].getTimeStamp() + "\" ";

	            // add checkpoints
	            highScores += "\"" + numCheckpoints+ "\" ";

	            for ( int j = 0; j < numCheckpoints; j++ )
	                highScores += "\"" + int( this.highScores[i].getCheckPoint(j) ) + "\" ";

	            highScores += "\n";
	        }
	    }

	    G_WriteFile( "gamedata/highscores/" + this.map.name, highScores );
	}

	/**
	 * Load the stats from the file
	 * @return void
	 */
	void loadStats()
	{
	    cString highScores;

	    highScores = G_LoadFile( "gamedata/highscores/" + this.map.name );

	    if ( highScores.len() > 0 )
	    {
	        cString timeToken, nameToken, dateToken, sectorToken;
	        int count = 0;

	        for ( int i = 0; i < MAX_RECORDS; i++ )
	        {
	            timeToken = highScores.getToken( count++ );
	            if ( timeToken.len() == 0 )
	                break;

	            nameToken = highScores.getToken( count++ );
	            if ( nameToken.len() == 0 )
	                break;
					
	            dateToken = highScores.getToken( count++ );
	            if ( dateToken.len() == 0 )
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

	            this.highScores[i].timeStamp = uint64( dateToken.toInt() );
	            this.highScores[i].finishTime = uint( timeToken.toInt() );
	            this.highScores[i].playerName = nameToken;
	        }

	        this.updateHud();
	    }
	}
	
	/**
	 * Update the stats in the user HUDs
	 * @return void
	 */
	void updateHud()
	{
	    for ( int i = 0; i < MAX_RECORDS; i++ )
	    {
	        if ( this.highScores[i].finishTime > 0 )
	        {
	            G_ConfigString( CS_GENERAL + i, "#" + ( i + 1 ) + " - "
					+ this.highScores[i].playerName + " - " + TimeToString( this.highScores[i].finishTime ) );
	        }
	    }
	}
	
	/**
	 * Get the stats as a string
	 * @return cString
	 */
	cString getStats()
	{
		cString stats = S_COLOR_ORANGE + "Top " + MAX_RECORDS + " players on map '"+ this.map.name + "' \n" + S_COLOR_WHITE;

	    for ( int i = 0; i < MAX_RECORDS; i++ )
	    {
	        int time = this.highScores[i].getTime();
			int bestTime = this.highScores[0].getTime();
			int difftime = time - bestTime;
			
			uint64 date = this.highScores[i].getTimeStamp();
			
			if ( time > 0 )
			{
				cString playerName = this.highScores[i].getPlayerName();
				// .42-like highscores
				stats += S_COLOR_WHITE + "" + (i+1) + ". " + S_COLOR_GREEN + TimeToString(time) + S_COLOR_YELLOW + "+[" + TimeToString(difftime) + "]   " + S_COLOR_WHITE + playerName + "  " +  S_COLOR_WHITE + "(" + DateToString(date) + ")\n";
			}
	    }

	    return stats;
	}
}