/**
 * Racesow custom entities file
 * based on Warsow 0.5 race gametype
 *
 * @package Racesow
 * @version 0.6.2
 */

cString[] entStorage(maxEntities);

void addToEntStorage( int id, cString string)
{
	int i = entStorage.length();
	if( i < id )
		entStorage.resize(id);
	entStorage[id] = string;
}

bool TriggerWait( cEntity @ent, cEntity @activator )
{
	if( @activator.client == null || @Racesow_GetPlayerByClient( activator.client ) == null )
		return false;
	Racesow_Player @player = @Racesow_GetPlayerByClient( activator.client );
	if( @player.triggerEntity == @ent && player.getTriggerTimeout() != 0
			&& player.getTriggerTimeout() >= levelTime )
		return true;
	player.setTriggerEntity( @ent );
	player.setTriggerTimeout( levelTime + 1000 * ent.wait );
	return false;
}

/*
 * Entity code for infinitive weapon pickup
 */

void replacementItem( cEntity @oldItem )
{
  	cVec3 min, max;
	cEntity @ent = @G_SpawnEntity( oldItem.getClassname() );
	cItem @item = @G_GetItem( oldItem.item.tag );
	@ent.item = @item;
	ent.setOrigin( oldItem.getOrigin() );
	oldItem.getSize( min, max );
	ent.setSize( min, max );
	ent.type = ET_ITEM;
	ent.solid = SOLID_TRIGGER;
	ent.moveType = MOVETYPE_NONE;
	ent.count = oldItem.count;
	ent.spawnFlags = oldItem.spawnFlags;
	ent.svflags &= ~SVF_NOCLIENT;
	ent.style = oldItem.style;
	ent.setTargetString( oldItem.getTargetString() );
	ent.setTargetnameString( oldItem.getTargetnameString() );
    ent.setupModel( oldItem.item.getModelString(), oldItem.item.getModel2String() );
	oldItem.solid = SOLID_NOT;
	oldItem.setClassname( "ASmodel_" + ent.item.getClassname() );
	ent.wait = oldItem.wait;

	if( ent.wait > 0 )
	{
        ent.nextThink = levelTime + ent.wait;
	}

	if( oldItem.item.type == uint(IT_WEAPON) )
	{
        ent.skinNum = oldItem.skinNum;
        oldItem.freeEntity();
	}
	ent.linkEntity();
}

void replacementItem_think( cEntity @ent )
{
    ent.respawnEffect();
}

void replacementItem_use( cEntity @ent, cEntity @other, cEntity @activator )
{
    if( ent.wait > 0 )
    {
        ent.nextThink = levelTime + ent.wait;
    }
    else
    {
        ent.nextThink = levelTime + 1;
    }
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
	int speed = entStorage[ent.entNum()].getToken(0).toInt();
	if(velocity.x == 0 && velocity.y == 0)
		return;
	velocity.x += (speed * velocity.x)/sqrt(pow(velocity.x, 2) + pow(velocity.y, 2));
	velocity.y += (speed * velocity.y)/sqrt(pow(velocity.x, 2) + pow(velocity.y, 2));
	velocity.z += entStorage[ent.entNum()].getToken(1).toInt();
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
        	G_FireGrenade( self.getOrigin(), angles, rs_grenadeweak_speed.getInteger(), 0, 65, rs_grenadeweak_knockback.getInteger(), 0, @activator );
            break;
        case WEAP_ROCKETLAUNCHER:
        	G_FireRocket( self.getOrigin(), angles, rs_rocketweak_speed.getInteger(), rs_rocketweak_splash.getInteger(), 75, rs_rocketweak_knockback.getInteger(), 0, @activator );
            break;
        case WEAP_PLASMAGUN:
        	G_FirePlasma( self.getOrigin(), angles, rs_plasmaweak_speed.getInteger(), rs_plasmaweak_splash.getInteger(), 15, rs_plasmaweak_knockback.getInteger(), 0, @activator );
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
		G_CenterPrintMsg( activator, entStorage[ent.entNum()] );
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

/*
 * Soundfix
 */
void AS_weapon_gunblade_use( cEntity @ent, cEntity @other, cEntity @activator )
{
    replacementItem_use( @ent, @other, @activator );
}

void AS_weapon_machinegun_use( cEntity @ent, cEntity @other, cEntity @activator )
{
    replacementItem_use( @ent, @other, @activator );
}

void AS_weapon_riotgun_use( cEntity @ent, cEntity @other, cEntity @activator )
{
    replacementItem_use( @ent, @other, @activator );
}

void AS_weapon_grenadelauncher_use( cEntity @ent, cEntity @other, cEntity @activator )
{
    replacementItem_use( @ent, @other, @activator );
}

void AS_weapon_rocketlauncher_use( cEntity @ent, cEntity @other, cEntity @activator )
{
    replacementItem_use( @ent, @other, @activator );
}

void AS_weapon_plasmagun_use( cEntity @ent, cEntity @other, cEntity @activator )
{
    replacementItem_use( @ent, @other, @activator );
}

void AS_weapon_lasergun_use( cEntity @ent, cEntity @other, cEntity @activator )
{
    replacementItem_use( @ent, @other, @activator );
}

void AS_weapon_electrobolt_use( cEntity @ent, cEntity @other, cEntity @activator )
{
    replacementItem_use( @ent, @other, @activator );
}

void AS_weapon_gunblade__think( cEntity @ent )
{
    replacementItem_think( @ent );
}

void AS_weapon_machinegun_think( cEntity @ent )
{
    replacementItem_think( @ent );
}

void AS_weapon_riotgun_think( cEntity @ent )
{
    replacementItem_think( @ent );
}

void AS_weapon_grenadelauncher_think( cEntity @ent )
{
    replacementItem_think( @ent );
}

void AS_weapon_rocketlauncher_think( cEntity @ent )
{
    replacementItem_think( @ent );
}

void AS_weapon_plasmagun_think( cEntity @ent )
{
    replacementItem_think( @ent );
}

void AS_weapon_lasergun_think( cEntity @ent )
{
    replacementItem_think( @ent );
}

void AS_weapon_electrobolt_think( cEntity @ent )
{
    replacementItem_think( @ent );
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

void AS_item_armor_ga_touch( cEntity @ent, cEntity @other, const cVec3 @planeNormal, int surfFlags )
{
	replacementItem_touch( @ent, @other );
}

void AS_item_armor_ya_touch( cEntity @ent, cEntity @other, const cVec3 @planeNormal, int surfFlags )
{
	replacementItem_touch( @ent, @other );
}

void AS_item_armor_ra_touch( cEntity @ent, cEntity @other, const cVec3 @planeNormal, int surfFlags )
{
	replacementItem_touch( @ent, @other );
}

void AS_item_armor_shard_touch( cEntity @ent, cEntity @other, const cVec3 @planeNormal, int surfFlags )
{
	//we don't want to have them.
}

void AS_item_health_small_touch( cEntity @ent, cEntity @other, const cVec3 @planeNormal, int surfFlags )
{
	//we don't want to have them.
}

void AS_item_health_medium_touch( cEntity @ent, cEntity @other, const cVec3 @planeNormal, int surfFlags )
{
	replacementItem_touch( @ent, @other );
}

void AS_item_health_large_touch( cEntity @ent, cEntity @other, const cVec3 @planeNormal, int surfFlags )
{
	replacementItem_touch( @ent, @other );
}

void AS_item_health_mega_touch( cEntity @ent, cEntity @other, const cVec3 @planeNormal, int surfFlags )
{
	replacementItem_touch( @ent, @other );
}

void AS_item_health_ultra_touch( cEntity @ent, cEntity @other, const cVec3 @planeNormal, int surfFlags )
{
	replacementItem_touch( @ent, @other );
}

void replacementItem_touch( cEntity @ent, cEntity @other )
{
	if( @other.client == null || other.moveType != MOVETYPE_PLAYER )
		return;
	if( ( other.client.pmoveFeatures & PMFEAT_ITEMPICK ) == 0 )
	    return;
	int count = other.client.inventoryCount( ent.item.tag );
	int inventoryMax = ent.item.inventoryMax;
	if( ( ent.item.type & IT_WEAPON ) == uint(IT_WEAPON) )
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
	else if( ( ent.item.type & IT_AMMO ) == uint(IT_AMMO) )
	{
		if( count >= inventoryMax )
			return;
		other.client.inventorySetCount( ent.item.tag, inventoryMax );
	}
	else if( ( ent.item.type & IT_ARMOR ) == uint(IT_ARMOR) )
	{
		if( other.client.armor >= ent.item.quantity )
			return;
		int amount = ( ent.count == 0 ) ? ent.item.quantity : ent.count;
		other.client.armor = amount;
	}
	else if( ( ent.item.type & IT_POWERUP ) == uint(IT_POWERUP) )
	{
		if( count > 0 )
			return;
		int amount = ( ent.count == 0 ) ? ent.item.quantity : ent.count;
		other.client.inventorySetCount( ent.item.tag, amount );
	}
	else if( ( ent.item.type & IT_HEALTH ) == uint(IT_HEALTH) )
	{
	    int healthAmount;
	    switch( ent.item.tag )
	    {
	    case HEALTH_SMALL:
	        healthAmount = 40;
	        break;
	    case HEALTH_MEDIUM:
	        healthAmount = 75;
	        break;
	    case HEALTH_LARGE:
	        healthAmount = 100;
	        break;
	    case HEALTH_MEGA:
	        healthAmount = 200;
	        break;
	    case HEALTH_ULTRA:
	        healthAmount = 200;
	        break;
	    }
        if( other.health >= 100 && healthAmount <= 100 ) 
            return;
        if( other.health >= 200 && healthAmount <= 200 ) 
            return;
        if( healthAmount <= other.health )
           return;
        other.health = healthAmount;
	}
	G_Sound( other, CHAN_ITEM, G_SoundIndex( ent.item.getPickupSoundString() ), 0.875 );
}
