/*
There is still some work needed to be done, 
but the basic functions of the Accuracy entities work properly.

Mostly printing the frags/fraglimit isn't done yet.
*/

int[] scoreCounter(maxClients);

//Hardcoded spawnFlags for target_fragsFilter
int REMOVER = 1;
int RUNONCE = 2;
int SILENT = 4;
int RESET = 8;

void target_fragsFilter_think( cEntity @ent )
{
    int i;

    for( i = 0; i < maxClients; i++ )
    {
        if( @G_GetClient(i) != null )
            ent.use( ent, G_GetClient(i).getEnt() );
    }
    ent.nextThink = levelTime + 1;
}

void target_fragsFilter_use( cEntity @ent, cEntity @other, cEntity @activator )
{
    int frags = entStorage[ent.entNum()].getToken(0).toInt();

    if(@activator == null || (activator.svflags & SVF_NOCLIENT) == 1 || @activator.client == null
        || @Racesow_GetPlayerByClient( activator.client ) == null )
        return;

	  if( frags <= 0 )
	  {
	      frags = 1;
	  }

		if( scoreCounter[activator.client.playerNum()] > frags )
			scoreCounter[activator.client.playerNum()] = frags;

    if(	scoreCounter[activator.client.playerNum()] == frags)
    {
				//Debug print
        //G_Print( "Fraglimit reached\n" );
        ent.useTargets( activator );
    }
    if( ( ent.spawnFlags & REMOVER ) > 0 )
    {
        scoreCounter[activator.client.playerNum()] -= frags;
    }
    if( ( ent.spawnFlags & RUNONCE ) > 0 )
    {
        if(	scoreCounter[activator.client.playerNum()] == frags)
            scoreCounter[activator.client.playerNum()] = 0;
    }
    if( ( ent.spawnFlags & SILENT ) > 0 )
    {
    }
    else
    {
				//FIXME: This needs to be printed somewhere else. Maybe add it to the hud.
        //activator.client.printMessage( "You have " + scoreCounter[activator.client.playerNum()] + "/" + frags + "\n" );
    }
    if( ( ent.spawnFlags & RESET ) > 0 )
    {
        scoreCounter[activator.client.playerNum()] = 0;
    }
}

/*QUAKED target_fragsFilter (1 0 0) (-8 -8 -8) (8 8 8) REMOVER RUNONCE SILENT RESET
Frags Filter
-------- KEYS --------
frags: (default is 1) number of frags required to trigger the targeted entity.
target: targeted entity.
-------- SPAWNFLAGS --------
REMOVER: removes from player's score the number of frags that was required to trigger the targeted entity.
RUNONCE: no longer used, kept for compatibility.
SILENT: disables player warnings. ("x more frags needed" messages)
RESET: resets player's score to 0 after the targeted entity is triggered.
-------- NOTES --------
If the Frags Filter is not bound from a trigger, it becomes independant and is so always active.
Defrag is limited to 10 independant target_fragsFilter.
*/

void target_fragsFilter( cEntity @ent )
{
  cString frags = G_SpawnTempValue("frags");

	addToEntStorage( ent.entNum(), frags );

	if( @ent.findTargetingEntity( null ) == null )
  {
      ent.spawnFlags |= SILENT;
      ent.nextThink = levelTime + 1;
  }

	//Debug prints
  /*G_Print( "REMOVER: " + ( ent.spawnFlags & REMOVER ) + "\n" );
  G_Print( "RUNONCE: " + ( ent.spawnFlags & RUNONCE ) + "\n" );
  G_Print( "SILENT: " + ( ent.spawnFlags & SILENT ) + "\n" );
  G_Print( "RESET: " + ( ent.spawnFlags & RESET ) + "\n" );*/
}

/*
============
AddScore

Adds score to both the client and his team
============
*/
void AddScore( cEntity @ent, cVec3 @origin, int score ) {
	if(@ent == null || (ent.svflags & SVF_NOCLIENT) == 1 || @ent.client == null
			|| @Racesow_GetPlayerByClient( ent.client ) == null)
        return;

	// I'm not sure what this did in Defrag
	// show score plum
	//ScorePlum(ent, origin, score);
	
	//FIXME: If there is only need for < 256 scores we could use a char
	if( ( scoreCounter[ent.client.playerNum()] + score ) > 300 )
	{
		scoreCounter[ent.client.playerNum()] = 300;
		return;
	}

	scoreCounter[ent.client.playerNum()] += score;

	// Team behaviour not yet ported to AS
	/*if ( g_gametype.integer == GT_TEAM )
		level.teamScores[ ent->client->ps.persistant[PERS_TEAM] ] += score;*/

	//Debug print
  //ent.client.printMessage( "Your score is: " + scoreCounter[ent.client.playerNum()] + "\n" );

	//CalculateRanks();
}

void target_score( cEntity @ent )
{
  if ( ent.count <= 0 )
  {
		ent.count = 1;
	}
}

void target_score_use( cEntity @ent, cEntity @other, cEntity @activator )
{
    if(@activator == null || (activator.svflags & SVF_NOCLIENT) == 1 || @activator.client == null
		|| @Racesow_GetPlayerByClient( activator.client ) == null )
        return;

    AddScore( activator, ent.getOrigin(), ent.count);
    ent.useTargets( activator );
}
