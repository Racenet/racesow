/*
There is still some work needed to be done, 
but the basic functions of the Accuracy entities work properly.

Mostly printing the frags/fraglimit isn't done yet.
*/

int[] scoreCounter(maxClients);
bool isAccuracyMap=false;

//Hardcoded spawnFlags for target_fragsFilter
int REMOVER = 1;
int RUNONCE = 2;
int SILENT = 4;
int RESET = 8;

void checkForAccuracyMap()
{
    int position;
    isAccuracyMap = false;

	if( @map == null )
	{
        //Debug Print
		//G_Print("Map is null\n");
		return;
	}

	if( G_FileLength( "scripts/" + map.name + ".defi" ) > 0 )
	{
		String defifile;
        String style;
		defifile = G_LoadFile( "scripts/" + map.name + ".defi" );

        //Debug Print
		//G_Print(defifile + "\n");

        if( ( position = defifile.locate("style",0) ) > -1 ) //FIXME: Locate was changed this needs to be fixed -K1ll
        {
            style = defifile.substr( position, defifile.len() );
            if( ( position = style.locate("\n", 0 ) ) > -1 ) //FIXME: Locate was changed this needs to be fixed -K1ll
            {
                style = style.substr( 0, position );
                //Debug Print
                //G_Print(style + "\n");
                if( style.locate("accuracy", 0) > -1 ) //FIXME: Locate was changed this needs to be fixed -K1ll
		        {
			        isAccuracyMap = true;
		        }
            }
        }
	}
}

void target_fragsFilter_think( cEntity @ent )
{
    int i;

    for( i = 0; i < maxClients; i++ )
    {
        if( @G_GetClient(i) != null )
            ent.use( ent, ent, G_GetClient(i).getEnt() );
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
		//FIXME: This needs to be printed somewhere else. Maybe add it to the hud. (This function is called every millisecond keep that in mind!)
		/*if( isAccuracyMap )
		{
           	activator.client.printMessage( "You have " + scoreCounter[activator.client.playerNum()] + "/" + frags + "\n" );
	    }*/
    }
    else
    {
        //Maybe add centerprint here
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
    String frags = G_SpawnTempValue("frags");

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
fragsFilter_addScore

Adds score to both the client and his team in the fragfilter
============
*/
void fragsFilter_addScore( cEntity @ent, Vec3 origin, int score ) {
	if(@ent == null || (ent.svflags & SVF_NOCLIENT) == 1 || @ent.client == null
		|| @Racesow_GetPlayerByClient( ent.client ) == null)
        return;

	// I'm not sure what this did in Defrag
	// show score plum
	//ScorePlum(ent, origin, score);
	
	//Just setting it to an reasonable limit for now. Maybe i'll make use of the full int later ;)
	if( ( scoreCounter[ent.client.playerNum()] + score ) > 500 )
	{
		scoreCounter[ent.client.playerNum()] = 500;
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

    fragsFilter_addScore( activator, ent.getOrigin(), ent.count);
	//Debug print
    /*if( isAccuracyMap )
        activator.client.printMessage( "You have " + scoreCounter[activator.client.playerNum()] + " frags\n" );*/
    ent.useTargets( activator );
}
