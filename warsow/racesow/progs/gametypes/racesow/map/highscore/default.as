/**
 * Racesow_Map_HighScore_Default
 *
 * Stores stats in a simple text file
 *
 * @package Racesow
 * @subpackage Map_HighScore
 * @version 0.5.5
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
		RS_MysqlInsertRace(race.getPlayer().getClient().getEnt(), race.getPlayer().getId(), race.getPlayer().getNickId(), map.getId(), race.getTime());
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
		// removed for now - r2
		int i_like_to_see_top_in_hud=0;
		if (i_like_to_see_top_in_hud==1)
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