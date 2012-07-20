/**
 * Racesow custom entities file
 * based on Warsow 0.5 race gametype
 *
 * @package Racesow
 * @version 0.6.2
 */

String[] entStorage(maxEntities);

void addToEntStorage( int id, String string)
{
	int i = entStorage.length();
	if( i < id )
		entStorage.resize(id);
	entStorage[id] = string;
}

bool TriggerWait( cEntity @ent, cEntity @activator )
{
	if( @activator.client == null || @racesowGametype.getPlayer( activator.client ) == null )
		return false;
	Racesow_Player @player = @racesowGametype.getPlayer( activator.client );
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
  	Vec3 min, max;
	cEntity @ent = @G_SpawnEntity( oldItem.classname );
	cItem @item = @G_GetItem( oldItem.item.tag );
	@ent.item = @item;
	ent.origin = oldItem.origin;
	oldItem.getSize( min, max );
	ent.setSize( min, max );
	ent.type = ET_ITEM;
	ent.solid = SOLID_TRIGGER;
	ent.moveType = MOVETYPE_NONE;
	ent.count = oldItem.count;
	ent.spawnFlags = oldItem.spawnFlags;
	ent.svflags &= ~SVF_NOCLIENT;
	ent.style = oldItem.style;
	ent.target = oldItem.target;
	ent.targetname = oldItem.targetname;
    ent.setupModel( oldItem.item.model, oldItem.item.model2 );
	oldItem.solid = SOLID_NOT;
	oldItem.classname = "ASmodel_" + ent.item.classname;
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
	@ent.think = replacementItem_think;
	@ent.touch = replacementItem_touch;
	@ent.use = replacementItem_use;
	ent.linkEntity();
}

void replacementItem_think( cEntity @ent )
{
    ent.respawnEffect();
}

/*
 * Soundfix
 */
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
	@ent.think = trigger_push_velocity_think;

	//@ent.enemy = @ent.findTargetEntity( ent );
	String speed = G_SpawnTempValue("speed");
	String count = G_SpawnTempValue("count");
	addToEntStorage( ent.entNum, speed + " " + count );
	ent.solid = SOLID_TRIGGER;
	ent.moveType = MOVETYPE_NONE;
    ent.setupModel(ent.model);
	ent.svflags &= ~SVF_NOCLIENT;
	ent.svflags |= SVF_TRANSMITORIGIN2/*|SVF_NOCULLATORIGIN2 Removed in Warsow 0.7*/;
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

void trigger_push_velocity_touch( cEntity @ent, cEntity @other, const Vec3 planeNormal, int surfFlags )
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
	Vec3 dir, velocity;
	//if(( ent.spawnFlags & 1 ) == 0 )
	velocity = other.velocity;
	if( velocity.length() == 0 || other.type != ET_PLAYER || other.moveType != MOVETYPE_PLAYER )
		return;
	if(TriggerWait( @ent, @other ))
			return;
	int speed = entStorage[ent.entNum].getToken(0).toInt();
	if(velocity.x == 0 && velocity.y == 0)
		return;
	velocity.x += (speed * velocity.x)/sqrt(pow(velocity.x, 2) + pow(velocity.y, 2));
	velocity.y += (speed * velocity.y)/sqrt(pow(velocity.x, 2) + pow(velocity.y, 2));
	velocity.z += entStorage[ent.entNum].getToken(1).toInt();
	other.velocity = velocity;

}

void target_teleporter_think( cEntity @ent )
{
	//set up the targets
	if(@ent.findTargetEntity(null) != null)
		@ent.enemy = @ent.findTargetEntity(null);
}

void target_teleporter( cEntity @ent )
{
	@ent.think = target_teleporter_think;
	@ent.use = target_teleporter_use;
	ent.nextThink = levelTime + 1; //set up the targets
	ent.wait = 1;
}

void target_teleporter_use( cEntity @ent, cEntity @other, cEntity @activator )
{
	if(TriggerWait( @ent, @activator ))
		return;
	if(@activator == null || (activator.svflags & SVF_NOCLIENT) == 1 || @activator.client == null
			|| @racesowGametype.getPlayer( activator.client ) == null || !TriggerWait(@ent, @activator)
			|| @ent.enemy == null)
		return;
	racesowGametype.getPlayer( activator.client ).teleport(ent.enemy.origin, ent.enemy.angles, true, true);

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

    @ent.think = target_delay_think;
    @ent.use = target_delay_use;
    if ( ent.wait == 0 )
    {
        ent.wait = 1;
    }
}

Cvar rs_plasmaweak_speed( "rs_plasmaweak_speed", "2400", CVAR_ARCHIVE );
Cvar rs_plasmaweak_knockback( "rs_plasmaweak_knockback", "14", CVAR_ARCHIVE );
Cvar rs_plasmaweak_splash( "rs_plasmaweak_splash", "45", CVAR_ARCHIVE );
Cvar rs_rocketweak_speed( "rs_rocketweak_speed", "1150", CVAR_ARCHIVE );
Cvar rs_rocketweak_knockback( "rs_rocketweak_knockback", "100", CVAR_ARCHIVE );
Cvar rs_rocketweak_splash( "rs_rocketweak_splash", "140", CVAR_ARCHIVE );
Cvar rs_grenadeweak_speed( "rs_grenadeweak_speed", "900", CVAR_ARCHIVE );
Cvar rs_grenadeweak_knockback( "rs_grenadeweak_knockback", "90", CVAR_ARCHIVE );
Cvar rs_grenadeweak_splash( "rs_grenadeweak_splash", "160", CVAR_ARCHIVE );

//==============
//RS_UseShooter
//==============
void RS_UseShooter( cEntity @self, cEntity @other, cEntity @activator ) {

	Vec3 dir;
	Vec3 angles;

    if ( @self.enemy != null ) {
        dir = self.enemy.origin - self.origin;
        dir.normalize();
    } else {
        dir = self.movedir;
        dir.normalize();
    }
    angles = dir.toAngles();
	switch ( self.weapon )
	{
        case WEAP_GRENADELAUNCHER:
        	G_FireGrenade( self.origin, angles, rs_grenadeweak_speed.integer, 0, 65, rs_grenadeweak_knockback.integer, 0, @activator );
            break;
        case WEAP_ROCKETLAUNCHER:
        	G_FireRocket( self.origin, angles, rs_rocketweak_speed.integer, rs_rocketweak_splash.integer, 75, rs_rocketweak_knockback.integer, 0, @activator );
            break;
        case WEAP_PLASMAGUN:
        	G_FirePlasma( self.origin, angles, rs_plasmaweak_speed.integer, rs_plasmaweak_splash.integer, 15, rs_plasmaweak_knockback.integer, 0, @activator );
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
    // target might be a moving object, so we can't set a movedir for it
    if ( self.targetname != "" ) {
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
    @ent.think = RS_InitShooter_Finish;
    @ent.use = RS_UseShooter;
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
    @ent.use = target_smallprint_use;
	String message = G_SpawnTempValue("message");
    if( message == "" )
    {
    	ent.freeEntity();
    }
    else
    {
    	addToEntStorage( ent.entNum, message );
    }
}

void target_smallprint_use( cEntity @ent, cEntity @other, cEntity @activator )
{
	if(@activator.client != null)
		G_CenterPrintMsg( activator, entStorage[ent.entNum] );
}

void target_kill( cEntity @ent )
{
    @ent.use = target_kill_use;
	//the rest does the use code
}

void target_kill_use( cEntity @ent, cEntity @other, cEntity @activator )
{
	activator.sustainDamage( @activator, null, Vec3(0,0,0), 9999, 0, 0, MOD_SUICIDE );
	activator.health = 0;
}

void target_relay_use( cEntity @ent, cEntity @other, cEntity @activator )
{
	ent.useTargets( @activator );
}

void target_relay( cEntity @ent )
{
    @ent.use = target_relay_use;
	//the rest does the use code
}

//FIXME: The touch functions for armor shards and small health explicitly weren't called before.
//Do we still want that behaviour? If yes please add it here -K1ll
void replacementItem_touch( cEntity @ent, cEntity @other, const Vec3 planeNormal, int surfFlags )
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
	G_Sound( other, CHAN_ITEM, G_SoundIndex( ent.item.pickupSound ), 0.875 );
}
