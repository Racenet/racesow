/**
 * Racesow_Map
 *
 * @package Racesow
 * @version 0.5.1a
 * @author soh-zolex <zolex@warsow-race.net>
 */
class Racesow_Map
{
	cString name;

	Racesow_Map_HighScore_Default @statsHandler;
	
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
	
	Racesow_Map_HighScore_Abstract @getStatsHandler()
	{
		return @this.statsHandler;
	}
}