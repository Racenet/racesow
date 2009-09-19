/**
 * Racesow_Map
 *
 */
class Racesow_Map
{
	cString name;

	Racesow_Map_HighScore_Default[] highScores;
	
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
		
		this.highScores.resize( MAX_RECORDS );
		for ( int i = 0; i < MAX_RECORDS; i++ )
		{
			this.highScores[i].reset();
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

	    G_WriteFile( "highScores/race/" + this.name + ".txt", highScores );
	}

	/**
	 * LoadHighScores
	 *
	 */
	void loadHighScores()
	{
	    cString highScores;

	    highScores = G_LoadFile( "highScores/race/" + this.name + ".txt" );

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
}