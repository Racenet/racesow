/**
 * Racesow custom entities file
 * based on Warsow 0.5 race gametype
 *
 * @package Racesow
 * @version 0.5.3
 */

cString[] ent_storage(maxEntities);

void addToEntStorage( int id, cString string)
{
	int i = ent_storage.length();
	if( i < id )
		ent_storage.resize(id);
	ent_storage[id] = string;
}

bool TriggerWait( cEntity @ent, cEntity @activator )
{
	if( @activator.client == null || @Racesow_GetPlayerByClient( activator.client ) == null )
		return false;
	Racesow_Player @player = @Racesow_GetPlayerByClient( activator.client );
	if( @player.getTrigger_Entity() == @ent && player.getTrigger_Timeout() != 0
			&& player.getTrigger_Timeout() >= levelTime )
		return true;
	player.setTrigger_Entity( @ent );
	player.setTrigger_Timeout( levelTime + 1000 * ent.wait );
	return false;
}

void replacementItem( cEntity @oldItem )
{
	cEntity @ent = @G_SpawnEntity( oldItem.getClassname() );
	@ent.item = @oldItem.item;
	ent.setOrigin( oldItem.getOrigin() );
	ent.setSize(cVec3(-16, -16, -16) , cVec3(16, 16, 40));
	//ent.modelindex = G_ModelIndex( ent.item.getModelString() );
	//ent.modelindex2 = G_ModelIndex( ent.item.getModel2String() );
	ent.solid = SOLID_TRIGGER;
	ent.moveType = MOVETYPE_TOSS;
	//ent.svflags &= ~SVF_NOCLIENT;
	//ent.effects = EF_ROTATE_AND_BOB;
	ent.count = oldItem.count;
	ent.spawnFlags = oldItem.spawnFlags;
	//ent.type = oldItem.type;
	oldItem.solid = SOLID_NOT;
	oldItem.setClassname( "ASmodel_" + ent.item.getClassname() );
	//oldItem.freeEntity();
	ent.linkEntity();
	if(( ent.spawnFlags & 1 ) == 1)
		ent.moveType = MOVETYPE_FLY;
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

/**
 * trigger_push_velocity
 * @param cEntity @ent
 * @return void
 */
void trigger_push_velocity( cEntity @ent )
{

	//@ent.enemy = @ent.findTargetEntity( ent );
	cString speed = G_SpawnTempValue("speed");
	cString count = G_SpawnTempValue("count");
	addToEntStorage( ent.entNum(), speed + " " + count );
	ent.solid = SOLID_TRIGGER;
	ent.moveType = MOVETYPE_NONE;
    ent.setupModel(ent.getModelString());
	ent.svflags &= ~SVF_NOCLIENT;
	ent.svflags |= SVF_TRANSMITORIGIN2|SVF_NOCULLATORIGIN2;
	ent.wait = 1;
	ent.linkEntity();
}

void trigger_push_velocity_think( cEntity @ent )
{
}

int PLAYERDIR_XY = 1;//apply the horizontal speed in the player's horizontal direction of travel, otherwise it uses the target XY component.
int ADD_XY = 2;//add to the player's horizontal velocity, otherwise it set's the player's horizontal velociy.
int PLAYERDIR_Z = 3;//apply the vertical speed in the player's vertical direction of travel, otherwise it uses the target Z component.
int ADD_Z = 4;//add to the player's vertical velocity, otherwise it set's the player's vectical velociy.
int BIDIRECTIONAL_XY = 5;//non-playerdir velocity pads will function in 2 directions based on the target specified.  The chosen direction is based on the current direction of travel.  Applies to horizontal direction.
int BIDIRECTIONAL_Z = 6;//non-playerdir velocity pads will function in 2 directions based on the target specified.  The chosen direction is based on the current direction of travel.  Applies to vertical direction.
int CLAMP_NEGATIVE_ADDS = 7;//adds negative velocity will be clamped to 0, if the resultant velocity would bounce the player in the opposite direction.

void trigger_push_velocity_touch( cEntity @ent, cEntity @other, const cVec3 @planeNormal, int surfFlags )
{
/*
	-------- KEYS --------
	target: this points to the target_position to which the player will jump.
	speed:
	count:
	-------- SPAWNFLAGS --------
	PLAYERDIR_XY: if set, trigger will apply the horizontal speed in the player's horizontal direction of travel, otherwise it uses the target XY component.
	ADD_XY: if set, trigger will add to the player's horizontal velocity, otherwise it set's the player's horizontal velociy.
	PLAYERDIR_Z: if set, trigger will apply the vertical speed in the player's vertical direction of travel, otherwise it uses the target Z component.
	ADD_Z: if set, trigger will add to the player's vertical velocity, otherwise it set's the player's vectical velociy.
	BIDIRECTIONAL_XY: if set, non-playerdir velocity pads will function in 2 directions based on the target specified.  The chosen direction is based on the current direction of travel.  Applies to horizontal direction.
	BIDIRECTIONAL_Z: if set, non-playerdir velocity pads will function in 2 directions based on the target specified.  The chosen direction is based on the current direction of travel.  Applies to vertical direction.
	CLAMP_NEGATIVE_ADDS: if set, then a velocity pad that adds negative velocity will be clamped to 0, if the resultant velocity would bounce the player in the opposite direction.
*/
	cVec3 dir, velocity;
	//if(( ent.spawnFlags & 1 ) == 0 )
	velocity = other.getVelocity();
	if( velocity.length() == 0 || other.type != ET_PLAYER || other.moveType != MOVETYPE_PLAYER )
		return;
	if(TriggerWait( @ent, @other ))
			return;
	int speed = ent_storage[ent.entNum()].getToken(0).toInt();
	if(velocity.x == 0 && velocity.y == 0)
		return;
	velocity.x += (speed * velocity.x)/sqrt(pow(velocity.x, 2) + pow(velocity.y, 2));
	velocity.y += (speed * velocity.y)/sqrt(pow(velocity.x, 2) + pow(velocity.y, 2));
	velocity.z += ent_storage[ent.entNum()].getToken(1).toInt();
	other.setVelocity( velocity );

}

void target_teleporter_think( cEntity @ent )
{
	//set up the targets
	if(@ent.findTargetEntity(null) != null)
		@ent.enemy = @ent.findTargetEntity(null);
}

void target_teleporter( cEntity @ent )
{
	ent.nextThink = levelTime + 1; //set up the targets
	ent.wait = 1;
}

void target_teleporter_use( cEntity @ent, cEntity @other, cEntity @activator )
{
	if(TriggerWait( @ent, @activator ))
		return;
	if(@activator == null || (activator.svflags & SVF_NOCLIENT) == 1 || @activator.client == null
			|| @Racesow_GetPlayerByClient( activator.client ) == null || !TriggerWait(@ent, @activator)
			|| @ent.enemy == null)
		return;
	Racesow_GetPlayerByClient( activator.client ).teleport(ent.enemy.getOrigin(), ent.enemy.getAngles(), true, true);

}

//=================
//QUAKED target_delay (1 0 0) (-8 -8 -8) (8 8 8)
//"wait" seconds to pause before firing targets.
//=================
void target_delay_think( cEntity @ent )
{
    ent.useTargets( @ent.enemy );
}

void target_delay_use( cEntity @self, cEntity @other, cEntity @activator )
{
    self.nextThink = levelTime + self.wait * 1000;
    @self.enemy = @activator;
}

void target_delay( cEntity @ent ) {

    if ( ent.wait == 0 )
    {
        ent.wait = 1;
    }
}

cVar rs_plasmaweak_speed( "rs_plasmaweak_speed", "2400", CVAR_ARCHIVE );
cVar rs_plasmaweak_knockback( "rs_plasmaweak_knockback", "14", CVAR_ARCHIVE );
cVar rs_plasmaweak_splash( "rs_plasmaweak_splash", "45", CVAR_ARCHIVE );
cVar rs_rocketweak_speed( "rs_rocketweak_speed", "1150", CVAR_ARCHIVE );
cVar rs_rocketweak_knockback( "rs_rocketweak_knockback", "100", CVAR_ARCHIVE );
cVar rs_rocketweak_splash( "rs_rocketweak_splash", "140", CVAR_ARCHIVE );
cVar rs_grenadeweak_speed( "rs_grenadeweak_speed", "900", CVAR_ARCHIVE );
cVar rs_grenadeweak_knockback( "rs_grenadeweak_knockback", "90", CVAR_ARCHIVE );
cVar rs_grenadeweak_splash( "rs_grenadeweak_splash", "160", CVAR_ARCHIVE );

//==============
//RS_UseShooter
//==============
void RS_UseShooter( cEntity @self, cEntity @other, cEntity @activator ) {

	cVec3 dir;
	cVec3 angles;

    if ( @self.enemy != null ) {
        dir = self.enemy.getOrigin() - self.getOrigin();
        dir.normalize();
    } else {
        self.getMovedir( dir );
        dir.normalize();
    }
    dir.toAngles(angles);
	switch ( self.weapon )
	{
        case WEAP_GRENADELAUNCHER:
        	G_FireGrenade( self.getOrigin(), angles, rs_grenadeweak_speed.getInteger(), 0, 0, rs_grenadeweak_knockback.getInteger(), 0, @activator );
            break;
        case WEAP_ROCKETLAUNCHER:
        	G_FireRocket( self.getOrigin(), angles, rs_rocketweak_speed.getInteger(), rs_rocketweak_splash.getInteger(), 0, rs_rocketweak_knockback.getInteger(), 0, @activator );
            break;
        case WEAP_PLASMAGUN:
        	G_FirePlasma( self.getOrigin(), angles, rs_plasmaweak_speed.getInteger(), rs_plasmaweak_splash.getInteger(), 0, rs_plasmaweak_knockback.getInteger(), 0, @activator );
            break;
    }

}

//======================
//RS_InitShooter_Finish
//======================
void RS_InitShooter_Finish( cEntity @self )
{
	@self.enemy = @self.findTargetEntity(null);
    self.nextThink = 0;
}

//===============
//RS_InitShooter
//===============
void RS_InitShooter( cEntity @self, int weapon ) {
    self.weapon = weapon;
    self.setMovedir();
    // target might be a moving object, so we can't set a movedir for it
    if ( self.getTargetnameString() != "" ) {
        self.nextThink = levelTime + 500;
    }
    self.linkEntity();

}


void shooter_rocket_think( cEntity @ent )
{
	RS_InitShooter_Finish( @ent );
}

void shooter_plasma_think( cEntity @ent )
{
	RS_InitShooter_Finish( @ent );
}

void shooter_grenade_think( cEntity @ent )
{
	RS_InitShooter_Finish( @ent );
}

void shooter_rocket_use( cEntity @self, cEntity @other, cEntity @activator ) {
	RS_UseShooter( @self, @other, @activator );
}

void shooter_plasma_use( cEntity @self, cEntity @other, cEntity @activator ) {
	RS_UseShooter( @self, @other, @activator );
}

void shooter_grenade_use( cEntity @self, cEntity @other, cEntity @activator )
{
	RS_UseShooter( @self, @other, @activator );
}

//=================
//RS_shooter_rocket
//===============
void shooter_rocket( cEntity @ent ) {
    RS_InitShooter( @ent, WEAP_ROCKETLAUNCHER );
}

//=================
//RS_shooter_plasma
//===============
void shooter_plasma( cEntity @ent ) {
    RS_InitShooter( @ent, WEAP_PLASMAGUN );
}

//=================
//RS_shooter_grenade
//===============
void shooter_grenade( cEntity @ent ) {
    RS_InitShooter( @ent, WEAP_GRENADELAUNCHER );
}

void target_smallprint( cEntity @ent )
{
	cString message = G_SpawnTempValue("message");
    if( message == "" )
    {
    	ent.freeEntity();
    }
    else
    {
    	addToEntStorage( ent.entNum(), message );
    }
}

void target_smallprint_use( cEntity @ent, cEntity @other, cEntity @activator )
{
	if(@activator.client != null)
		G_CenterPrintMsg( activator, ent_storage[ent.entNum()] );
}

void target_kill( cEntity @ent )
{
	//the rest does the use code
}

void target_kill_use( cEntity @ent, cEntity @other, cEntity @activator )
{
	activator.takeDamage( @activator, null, cVec3(0,0,0), 9999, 0, 0, MOD_SUICIDE );
	activator.health = 0;
}

void target_relay_use( cEntity @ent, cEntity @other, cEntity @activator )
{
	ent.useTargets( @activator );
}

void target_relay( cEntity @ent )
{
	//the rest does the use code
}

void AS_weapon_gunblade_touch( cEntity @ent, cEntity @other, const cVec3 @planeNormal, int surfFlags )
{
	replacementItem_touch( @ent, @other );
}

void AS_weapon_machinegun_touch( cEntity @ent, cEntity @other, const cVec3 @planeNormal, int surfFlags )
{
	replacementItem_touch( @ent, @other );
}

void AS_weapon_riotgun_touch( cEntity @ent, cEntity @other, const cVec3 @planeNormal, int surfFlags )
{
	replacementItem_touch( @ent, @other );
}

void AS_weapon_grenadelauncher_touch( cEntity @ent, cEntity @other, const cVec3 @planeNormal, int surfFlags )
{
	replacementItem_touch( @ent, @other );
}

void AS_weapon_rocketlauncher_touch( cEntity @ent, cEntity @other, const cVec3 @planeNormal, int surfFlags )
{
	replacementItem_touch( @ent, @other );
}

void AS_weapon_plasmagun_touch( cEntity @ent, cEntity @other, const cVec3 @planeNormal, int surfFlags )
{
	replacementItem_touch( @ent, @other );
}

void AS_weapon_lasergun_touch( cEntity @ent, cEntity @other, const cVec3 @planeNormal, int surfFlags )
{
	replacementItem_touch( @ent, @other );
}

void AS_weapon_electrobolt_touch( cEntity @ent, cEntity @other, const cVec3 @planeNormal, int surfFlags )
{
	replacementItem_touch( @ent, @other );
}

void AS_ammo_gunblade_touch( cEntity @ent, cEntity @other, const cVec3 @planeNormal, int surfFlags )
{
	replacementItem_touch( @ent, @other );
}

void AS_ammo_machinegun_touch( cEntity @ent, cEntity @other, const cVec3 @planeNormal, int surfFlags )
{
	replacementItem_touch( @ent, @other );
}

void AS_ammo_riotgun_touch( cEntity @ent, cEntity @other, const cVec3 @planeNormal, int surfFlags )
{
	replacementItem_touch( @ent, @other );
}

void AS_ammo_grenadelauncher_touch( cEntity @ent, cEntity @other, const cVec3 @planeNormal, int surfFlags )
{
	replacementItem_touch( @ent, @other );
}

void AS_ammo_rocketlauncher_touch( cEntity @ent, cEntity @other, const cVec3 @planeNormal, int surfFlags )
{
	replacementItem_touch( @ent, @other );
}

void AS_ammo_plasmagun_touch( cEntity @ent, cEntity @other, const cVec3 @planeNormal, int surfFlags )
{
	replacementItem_touch( @ent, @other );
}

void AS_ammo_lasergun_touch( cEntity @ent, cEntity @other, const cVec3 @planeNormal, int surfFlags )
{
	replacementItem_touch( @ent, @other );
}

void AS_ammo_electrobolt_touch( cEntity @ent, cEntity @other, const cVec3 @planeNormal, int surfFlags )
{
	replacementItem_touch( @ent, @other );
}

void AS_item_quad_touch( cEntity @ent, cEntity @other, const cVec3 @planeNormal, int surfFlags )
{
	replacementItem_touch( @ent, @other );
}

void AS_item_warshell_touch( cEntity @ent, cEntity @other, const cVec3 @planeNormal, int surfFlags )
{
	replacementItem_touch( @ent, @other );
}

void replacementItem_touch( cEntity @ent, cEntity @other )
{
	if( @other.client == null || other.moveType != MOVETYPE_PLAYER )
		return;
	int count = other.client.inventoryCount( ent.item.tag );
	int inventoryMax = ent.item.inventoryMax;
	if( ( ent.item.type & IT_WEAPON ) == IT_WEAPON )
	{
		int weakcount = other.client.inventoryCount( ent.item.weakAmmoTag );
		int weakinventoryMax = G_GetItem( ent.item.weakAmmoTag ).inventoryMax;
		if( count >= inventoryMax && weakcount >= weakinventoryMax )
			return;
		if( count == 0 || other.client.canSelectWeapon( ent.item.tag ) )
			other.client.inventoryGiveItem( ent.item.tag, inventoryMax );
		other.client.inventorySetCount( ent.item.weakAmmoTag, weakinventoryMax );
		if( other.client.pendingWeapon == WEAP_GUNBLADE )
			other.client.selectWeapon( ent.item.tag );
	}
	else if( ( ent.item.type & IT_AMMO ) == IT_AMMO )
	{
		if( count >= inventoryMax )
			return;
		other.client.inventorySetCount( ent.item.tag, inventoryMax );
	}
	else
	{
		if( count > 0 )
			return;
		int amount = ( ent.count == 0 ) ? ent.item.quantity : ent.count;
		other.client.inventorySetCount( ent.item.tag, amount );
	}
	G_Sound( other, CHAN_ITEM, G_SoundIndex( ent.item.getPickupSoundString() ), 0.875 );
	ent.respawnEffect();
}
