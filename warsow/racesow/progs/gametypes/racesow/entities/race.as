/**
 * race_respawner_think
 * the player has finished the race. This entity times his automatic respawning
 * @param cEntity @respawner
 * @return void
 */
void race_respawner_think( cEntity @respawner )
{
    cClient @client = G_GetClient( respawner.count );

    // the client may have respawned on his own. If the last time was erased, don't respawn him
    if ( !Racesow_GetPlayerByClient( client ).isSpawned )
    {
        client.respawn( false );
        Racesow_GetPlayerByClient( client ).isSpawned = true;
    }

    respawner.freeEntity(); // free the respawner
}
// This sucks: some defrag maps have the entity classname with pseudo camel notation
// and classname->function is case sensitive so we need some shadow functions

/**
 * Cgg - defrag support
 * target_init are meant to reset the player hp, armor and inventory.
 * spawnflags can be used to limit the effects of the target to certain types of items :
 *   - spawnflag 1 prevents the armor from being removed.
 *   - spawnflag 2 prevents the hp from being reset.
 *   - spawnflag 4 prevents the weapons and ammo from being removed.
 *   - spawnflag 8 prevents the powerups from being removed.
 *   - spawnflag 16 used to prevent the removal of the holdable items (namely the
 *     medkit and teleport) from the player inventory.
 *
 * @param cEntity @self
 * @param cEntity @other
 * @param cEntity @activator
 * @return void
 */
void target_init_use( cEntity @self, cEntity @other, cEntity @activator )
{
    int i;

    if ( @activator.client == null )
        return;

    // armor
    if ( ( self.spawnFlags & 1 ) == 0 )
        activator.client.armor = 0;

    // health
    if ( ( self.spawnFlags & 2 ) == 0 )
    {
        activator.health = activator.maxHealth;
    }

    // weapons
    if ( ( self.spawnFlags & 4 ) == 0 )
    {
        for ( int i = WEAP_GUNBLADE; i < WEAP_TOTAL; i++ )
        {
            activator.client.inventorySetCount( i, 0 );
        }

        for ( int i = AMMO_WEAK_GUNBLADE; i < AMMO_TOTAL; i++ )
        {
            activator.client.inventorySetCount( i, 0 );
        }

        activator.client.inventorySetCount( WEAP_GUNBLADE, 1 );
        activator.client.selectWeapon( WEAP_GUNBLADE );
    }

    // powerups
    if ( ( self.spawnFlags & 8 ) == 0 )
    {
        for ( i = POWERUP_QUAD; i < POWERUP_TOTAL; i++ )
            activator.client.inventorySetCount( i, 0 );
    }
}

/**
 * target_init
 * doesn't need to do anything at all, just sit there, waiting
 * @param cEntity @self
 * @return void
 */
void target_init( cEntity @self )
{
}

/**
 * target_checkpoint_use
 * @param cEntity @self
 * @param cEntity @other
 * @param cEntity @activator
 * @return void
 */
void target_checkpoint_use( cEntity @self, cEntity @other, cEntity @activator )
{
    if ( @activator.client == null )
        return;

    if ( !Racesow_GetPlayerByClient( activator.client ).isRacing() )
        return;

    Racesow_GetPlayerByClient( activator.client ).touchCheckPoint( self.count );
}

/**
 * target_checkpoint
 * @param cEntity @self
 * @return void
 */
void target_checkpoint( cEntity @self )
{
    self.count = numCheckpoints;
    numCheckpoints++;
}

/**
 * target_stoptimer_use
 * @param cEntity @self
 * @param cEntity @other
 * @param cEntity @activator
 * @return void
 */
void target_stoptimer_use( cEntity @self, cEntity @other, cEntity @activator )
{
    if ( @activator.client == null )
        return;

    Racesow_GetPlayerByClient( activator.client ).touchStopTimer();
}

/**
 * target_stopTimer_use
 * defrag maps compatibility
 * @param cEntity @self
 * @param cEntity @other
 * @param cEntity @activator
 * @return void
 */
void target_stopTimer_use( cEntity @self, cEntity @other, cEntity @activator )
{
    target_stoptimer_use( self, other, activator );
}

/**
 * target_stoptimer
 * @param cEntity @self
 * @return void
 */
void target_stoptimer( cEntity @self )
{
}

/**
 * target_stopTimer
 * defrag maps compatibility
 * @param cEntity @self
 * @return void
 */
void target_stopTimer( cEntity @self )
{
}

/**
 * target_starttimer_use
 * @param cEntity @self
 * @param cEntity @other
 * @param cEntity @activator
 * @return void
 */
void target_starttimer_use( cEntity @self, cEntity @other, cEntity @activator )
{
    if ( @activator.client == null )
        return;

    Racesow_GetPlayerByClient( activator.client ).touchStartTimer();
}

/**
 * target_startTimer_use
 * defrag maps compatibility
 * @param cEntity @self
 * @param cEntity @other
 * @param cEntity @activator
 * @return void
 */
void target_startTimer_use( cEntity @self, cEntity @other, cEntity @activator )
{
    target_starttimer_use( self, other, activator );
}

/**
 *  target_starttimer
 * doesn't need to do anything at all, just sit there, waiting
 * @param cEntity @self
 * @return void
 */
void target_starttimer( cEntity @self )
{
}

/**
 * target_startTimer
 * defrag maps compatibility
 * @param cEntity @self
 * @return void
 */
void target_startTimer( cEntity @self )
{
    target_starttimer( self );
}