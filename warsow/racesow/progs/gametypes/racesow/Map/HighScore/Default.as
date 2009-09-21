/**
 * Racesow_Map_HighScore_Default
 *
 * Stores stats in a simple text file
 *
 * @package Racesow
 * @subpackage Map_HighScore
 * @version 0.5.1b
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
				// move the other records down
				for ( int i = MAX_RECORDS - 1; i > top; i-- )
				{
					if (this.highScores[i - 1].getTime() == 0)
						break;
						
					this.highScores[i] = this.highScores[i - 1];
				}

				this.highScores[top].fromRace( race );

				this.writeStats();
				this.updateHud();
				break;
			}
		}
	}
	
	void logRace(Racesow_Player_Race @race)
	{
		G_WriteFile( "gamedata/races/" + this.map.name + "_" + this.logId, race.toString() );
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
	            highScores += "\"" + time + "\" \"" + playerName + "\" ";

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
		cString stats = "";

	    for ( int i = 0; i < MAX_RECORDS; i++ )
	    {
	        int time = this.highScores[i].getTime();
			if ( time > 0 )
			{
				cString playerName = this.highScores[i].getPlayerName();
				stats += (i+1) + ". " + playerName + " ("+ TimeToString(time) +")\n";
			}
	    }

	    return stats;
	}
}