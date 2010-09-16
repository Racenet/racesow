/*
   Copyright (C) 1997-2001 Id Software, Inc.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

   See the GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

 */

#include "g_local.h"

//===============
//G_TriggerWait
//
//Called always when using a trigger that supports wait flag
//Returns true if the trigger shouldn't be activated
//===============
static qboolean G_TriggerWait( edict_t *ent, edict_t *other )
{
	if( GS_RaceGametype() )
	{
		if( other->trigger_entity == ent && other->trigger_timeout && other->trigger_timeout >= level.time )
			return qtrue;

		other->trigger_entity = ent;
		other->trigger_timeout = level.time + 1000 * ent->wait;
		return qfalse;
	}

	if( ent->timeStamp >= level.time )
		return qtrue;

	// the wait time has passed, so set back up for another activation
	ent->timeStamp = level.time + ( ent->wait * 1000 );
	return qfalse;
}

static void InitTrigger( edict_t *self )
{
	self->r.solid = SOLID_TRIGGER;
	self->movetype = MOVETYPE_NONE;
	GClip_SetBrushModel( self, self->model );
	self->r.svflags = SVF_NOCLIENT;
}


// the trigger was just activated
// ent->activator should be set to the activator so it can be held through a delay
// so wait for the delay time before firing
static void multi_trigger( edict_t *ent )
{
	if( G_TriggerWait( ent, ent->activator ) )
		return;		// already been triggered

	G_UseTargets( ent, ent->activator );

	if( ent->wait <= 0 && !GS_RaceGametype() ) //racesow
	{
		// we can't just remove (self) here, because this is a touch function
		// called while looping through area links...
		ent->touch = NULL;
		ent->nextThink = level.time + 1;
		ent->think = G_FreeEdict;
	}
}

static void Use_Multi( edict_t *ent, edict_t *other, edict_t *activator )
{
	ent->activator = activator;
	multi_trigger( ent );
}

static void Touch_Multi( edict_t *self, edict_t *other, cplane_t *plane, int surfFlags )
{
	if( other->r.client )
	{
		if( self->spawnflags & 2 )
			return;
	}
	else
		return;

	if( self->s.team && self->s.team != other->s.team )
		return;

	self->activator = other;
	multi_trigger( self );
}


//QUAKED trigger_multiple (.5 .5 .5) ? MONSTER NOT_PLAYER TRIGGERED
//Variable size repeatable trigger. It will fire the entities it targets when touched by player. Can be made to operate like a trigger_once entity by setting the "wait" key to -1. It can also be activated by another trigger that targets it.
//-------- KEYS --------
//target : this points to the entity to activate.
//targetname : activating trigger points to this.
//noise : play this noise when triggered
//message : centerprint this text string when triggered
//wait : time in seconds until trigger becomes re-triggerable after it's been touched (default 0.2, -1 = trigger once).
//notsingle : when set to 1, entity will not spawn in Single Player mode
//notfree : when set to 1, entity will not spawn in "Free for all" and "Tournament" modes.
//notduel : when set to 1, entity will not spawn in "Teamplay" and "CTF" modes. (jaltodo)
//notteam : when set to 1, entity will not spawn in "Teamplay" and "CTF" modes.
//notctf : when set to 1, entity will not spawn in "Teamplay" and "CTF" modes. (jaltodo)
//-------- SPAWNFLAGS --------
//MONSTER : &1 monsters won't activate this trigger unless this flag is set
//NOT_PLAYER : &2 players can't trigger this one (for those triggered by other triggers)
//TRIGGERED : &4 spawns as triggered and must wait for the "wait" key to pass to be re-triggered
//-------- NOTES --------
//message is untested

static void trigger_enable( edict_t *self, edict_t *other, edict_t *activator )
{
	self->r.solid = SOLID_TRIGGER;
	self->use = Use_Multi;
	GClip_LinkEntity( self );
}

void SP_trigger_multiple( edict_t *ent )
{
	GClip_SetBrushModel( ent, ent->model );
	G_PureModel( ent->model );

	if( st.noise )
	{
		ent->noise_index = trap_SoundIndex( st.noise );
		G_PureSound( st.noise );
	}

	// gameteam field from editor
	if( st.gameteam >= TEAM_SPECTATOR && st.gameteam < GS_MAX_TEAMS )
		ent->s.team = st.gameteam;
	else
		ent->s.team = TEAM_SPECTATOR;

	if( !ent->wait )
		ent->wait = 0.2f;

	ent->touch = Touch_Multi;
	ent->movetype = MOVETYPE_NONE;
	ent->r.svflags |= SVF_NOCLIENT;

	if( ent->spawnflags & 4 )
	{
		ent->r.solid = SOLID_NOT;
		ent->use = trigger_enable;
	}
	else
	{
		ent->r.solid = SOLID_TRIGGER;
		ent->use = Use_Multi;
	}

	GClip_LinkEntity( ent );
}


//QUAKED trigger_once (.5 .5 .5) ? MONSTER NOT_PLAYER TRIGGERED
//Triggers once, then removes itself. You must set the key "target" to the name of another object in the level that has a matching "targetname".
//-------- KEYS --------
//target : this points to the entity to activate.
//targetname : activating trigger points to this.
//noise : play this noise when triggered
//message : centerprint this text string when triggered
//notsingle : when set to 1, entity will not spawn in Single Player mode
//notfree : when set to 1, entity will not spawn in "Free for all" and "Tournament" modes.
//notduel : when set to 1, entity will not spawn in "Teamplay" and "CTF" modes. (jaltodo)
//notteam : when set to 1, entity will not spawn in "Teamplay" and "CTF" modes.
//notctf : when set to 1, entity will not spawn in "Teamplay" and "CTF" modes. (jaltodo)
//-------- SPAWNFLAGS --------
//MONSTER : &1 monsters won't activate this trigger unless this flag is set
//NOT_PLAYER : &2 players can't trigger this one (for those triggered by other triggers)
//TRIGGERED : &4 spawns as triggered and must wait for the "wait" key to pass to be re-triggered
//-------- NOTES --------
//Wait key will be ignored. message is untested

void SP_trigger_once( edict_t *ent )
{
	ent->wait = -1;
	SP_trigger_multiple( ent );
}

//QUAKED trigger_relay (.5 .5 .5) ? (-8 -8 -8) (8 8 8)
//This fixed size trigger cannot be touched, it can only be fired by other events.
//-------- KEYS --------
//target : this points to the entity to activate.
//targetname : activating trigger points to this.
//notsingle : when set to 1, entity will not spawn in Single Player mode
//notfree : when set to 1, entity will not spawn in "Free for all" and "Tournament" modes.
//notduel : when set to 1, entity will not spawn in "Teamplay" and "CTF" modes. (jaltodo)
//notteam : when set to 1, entity will not spawn in "Teamplay" and "CTF" modes.
//notctf : when set to 1, entity will not spawn in "Teamplay" and "CTF" modes. (jaltodo)
//-------- NOTES --------
//Trigger_relay is a tool for use in entities meccanos. It's of no use by itself, and can only be used as an intermediary between events. Wait key will be ignored.
static void trigger_relay_use( edict_t *self, edict_t *other, edict_t *activator )
{
	G_UseTargets( self, activator );
}

void SP_trigger_relay( edict_t *self )
{
	self->use = trigger_relay_use;
}

//==============================================================================
//
//trigger_counter
//
//==============================================================================

//QUAKED trigger_counter (.5 .5 .5) ? NOMESSAGE NOSOUNDS
//Acts as an intermediary for an action that takes multiple inputs. Example: a sequence of several buttons to activate a event
//-------- KEYS --------
//target : this points to the entity to activate.
//targetname : activating trigger points to this.
//count : number of actions to count (default 2)
//noise_start : sound to play each time a event happens
//noise_stop : sound to play at the last event in the count
//notsingle : when set to 1, entity will not spawn in Single Player mode
//notfree : when set to 1, entity will not spawn in "Free for all" and "Tournament" modes.
//notduel : when set to 1, entity will not spawn in "Teamplay" and "CTF" modes. (jaltodo)
//notteam : when set to 1, entity will not spawn in "Teamplay" and "CTF" modes.
//notctf : when set to 1, entity will not spawn in "Teamplay" and "CTF" modes. (jaltodo)
//-------- SPAWNFLAGS --------
//NOMESSAGE : &1 if not set, it will print "1 more.. " etc when triggered and "sequence complete" when finished.
//NOSOUNDS : &2 if not set, it will try to play the noise_start and noise_stop sounds
//-------- NOTES --------
//Sounds like this one should be a target and not a trigger, but well...
static void trigger_counter_use( edict_t *self, edict_t *other, edict_t *activator )
{
	if( self->count == 0 )
		return;

	self->count--;

	if( self->count )
	{
		if( !( self->spawnflags & 1 ) )
			G_CenterPrintMsg( activator, "%i more to go...", self->count );
		if( !( self->spawnflags & 2 ) )
			G_Sound( activator, CHAN_AUTO, self->moveinfo.sound_start, ATTN_NORM );

		return;
	}

	if( !( self->spawnflags & 1 ) )
		G_CenterPrintMsg( activator, "Sequence completed!" );
	if( !( self->spawnflags & 2 ) )
		G_Sound( activator, CHAN_AUTO, self->moveinfo.sound_end, ATTN_NORM );

	self->activator = activator;
	multi_trigger( self );
}

void SP_trigger_counter( edict_t *self )
{
	self->wait = -1;
	if( !self->count )
		self->count = 2;

	G_AssignMoverSounds( self, NULL, NULL, NULL );

	self->use = trigger_counter_use;
}


//==============================================================================
//
//trigger_always
//
//==============================================================================


//QUAKED trigger_always (.5 .5 .5) (-8 -8 -8) (8 8 8)
//Automatic trigger. It will fire the entities it targets as soon as it spawns in the game.
//-------- KEYS --------
//target : fire entities with this targetname.
//notsingle : when set to 1, entity will not spawn in Single Player mode
//notfree : when set to 1, entity will not spawn in "Free for all" and "Tournament" modes.
//notduel : when set to 1, entity will not spawn in "Teamplay" and "CTF" modes. (jaltodo)
//notteam : when set to 1, entity will not spawn in "Teamplay" and "CTF" modes.
//notctf : when set to 1, entity will not spawn in "Teamplay" and "CTF" modes. (jaltodo)

void SP_trigger_always( edict_t *ent )
{
	// we must have some delay to make sure our use targets are present
	if( ent->delay < 0.2f )
		ent->delay = 0.2f;
	G_UseTargets( ent, ent );
}


//==============================================================================
//
//trigger_push
//
//==============================================================================


//QUAKED trigger_push (.5 .5 .5) ? PUSH_ONCE
//This is used to create jump pads and launch ramps. It MUST point to a target_position or info_notnull entity to work.
//-------- KEYS --------
//target : this points to the target_position to which the player will jump.
//noise : override default noise ("silent" doesn't make any noise)
//wait : time before it can be triggered again.
//notsingle : when set to 1, entity will not spawn in Single Player mode
//notfree : when set to 1, entity will not spawn in "Free for all" and "Tournament" modes.
//notduel : when set to 1, entity will not spawn in "Teamplay" and "CTF" modes. (jaltodo)
//notteam : when set to 1, entity will not spawn in "Teamplay" and "CTF" modes.
//notctf : when set to 1, entity will not spawn in "Teamplay" and "CTF" modes. (jaltodo)
//-------- SPAWNFLAGS --------
//PUSH_ONCE : &1 only push when touched the first time
//-------- NOTES --------
//To make a jump pad or launch ramp, place the target_position/info_notnull entity at the highest point of the jump and target it with this entity.


static void G_JumpPadSound( edict_t *ent )
{
	vec3_t org;

	if( !ent->s.modelindex )
		return;

	if( !ent->moveinfo.sound_start )
		return;

	org[0] = ent->s.origin[0] + 0.5 * ( ent->r.mins[0] + ent->r.maxs[0] );
	org[1] = ent->s.origin[1] + 0.5 * ( ent->r.mins[1] + ent->r.maxs[1] );
	org[2] = ent->s.origin[2] + 0.5 * ( ent->r.mins[2] + ent->r.maxs[2] );

	G_PositionedSound( org, CHAN_AUTO, ent->moveinfo.sound_start, ATTN_NORM );
}

#define PUSH_ONCE	1
#define MIN_TRIGGER_PUSH_REBOUNCE_TIME 100

static void trigger_push_touch( edict_t *self, edict_t *other, cplane_t *plane, int surfFlags )
{
	if( self->s.team && self->s.team != other->s.team )
		return;

	if( G_TriggerWait( self, other ) )
		return;

	// add an event
	if( other->r.client )
	{
		if( other->r.client->ps.pmove.pm_type != PM_NORMAL )
			return;

		GS_TouchPushTrigger( &other->r.client->ps, &self->s );
	}
	else
	{  
		// pushing of non-clients
		if( other->movetype != MOVETYPE_BOUNCEGRENADE )
			return;

		// grenades have more air friction than players (weird, isn't it?), so we need some extra velocity
		//VectorCopy( self->s.origin2, other->velocity );
		VectorScale( self->s.origin2, 1.25, other->velocity );
	}

	// game timers for fall damage
	G_JumpPadSound( self ); // play jump pad sound

	// self removal
	if( self->spawnflags & PUSH_ONCE )
	{
		self->touch = NULL;
		self->nextThink = level.time + 1;
		self->think = G_FreeEdict;
	}
}

static void trigger_push_setup( edict_t *self )
{
	vec3_t origin, velocity;
	float height, time;
	float dist;
	edict_t	*target;

	if( !self->target )
	{
		vec3_t movedir;

		G_SetMovedir( self->s.angles, movedir );
		VectorScale( movedir, (self->speed ? self->speed : 1000) * 10, self->s.origin2 );
		return;
	}

	target = G_PickTarget( self->target );
	if( !target )
	{
		G_FreeEdict( self );
		return;
	}

	VectorAdd( self->r.absmin, self->r.absmax, origin );
	VectorScale( origin, 0.5, origin );

	height = target->s.origin[2] - origin[2];
	time = sqrt( height / ( 0.5 * g_gravity->value ) );
	if( !time )
	{
		G_FreeEdict( self );
		return;
	}

	VectorSubtract( target->s.origin, origin, velocity );
	velocity[2] = 0;
	dist = VectorNormalize( velocity );
	VectorScale( velocity, dist / time, velocity );
	velocity[2] = time * g_gravity->value;
	VectorCopy( velocity, self->s.origin2 );
}

void SP_trigger_push( edict_t *self )
{
	InitTrigger( self );

	if( st.noise && Q_stricmp( st.noise, "default" ) )
	{
		if( Q_stricmp( st.noise, "silent" ) )
		{
			self->moveinfo.sound_start = trap_SoundIndex( st.noise );
			G_PureSound( st.noise );
		}
	}
	else
		self->moveinfo.sound_start = trap_SoundIndex( S_JUMPPAD );

	// gameteam field from editor
	if( st.gameteam >= TEAM_SPECTATOR && st.gameteam < GS_MAX_TEAMS )
		self->s.team = st.gameteam;
	else
		self->s.team = TEAM_SPECTATOR;

	if( G_IsQ1Map() )
		self->wait = 0;

	self->touch = trigger_push_touch;
	self->think = trigger_push_setup;
	self->nextThink = level.time + 1;
	self->r.svflags &= ~SVF_NOCLIENT;
	self->s.type = ET_PUSH_TRIGGER;
	self->r.svflags |= SVF_TRANSMITORIGIN2|SVF_NOCULLATORIGIN2;
	GClip_LinkEntity( self ); // ET_PUSH_TRIGGER gets exceptions at linking so it's added for prediction
	self->timeStamp = level.time;
	if( !self->wait )
		self->wait = MIN_TRIGGER_PUSH_REBOUNCE_TIME * 0.001f;
}

void Use_target_push( edict_t *self, edict_t *other, edict_t *activator ) {
	if ( !activator->r.client ) {
		return;
	}

	if( activator->r.client->ps.pmove.pm_type != PM_NORMAL )
		return;

	VectorCopy (self->s.origin2, activator->velocity);

	// play fly sound every 1.5 seconds - No sound yet.
	/*if ( activator->fly_sound_debounce_time < level.time ) {
		activator->fly_sound_debounce_time = level.time + 1500;
		G_Sound( activator, CHAN_AUTO, self->noise_index, ATTN_NORM );
	}*/
}

/*QUAKED target_push (.5 .5 .5) (-8 -8 -8) (8 8 8) bouncepad
Pushes the activator in the direction.of angle, or towards a target apex.
"speed"		defaults to 1000
if "bouncepad", play bounce noise instead of windfly
*/
void SP_target_push( edict_t *self ) {
	if (!self->speed) {
		self->speed = 1000;
	}

	G_SetMovedir (self->s.angles, self->s.origin2);
	VectorScale (self->s.origin2, self->speed, self->s.origin2);

	// No sound yet.
	/*if ( self->spawnflags & 1 ) {
		self->noise_index = "sound/world/jumppad.wav";
	} else {
		self->noise_index = "sound/misc/windfly.wav";
	}*/
	if ( self->target ) {
		VectorCopy( self->s.origin, self->r.absmin );
		VectorCopy( self->s.origin, self->r.absmax );
		self->r.svflags |= SVF_TRANSMITORIGIN2|SVF_NOCULLATORIGIN2;
		self->think = trigger_push_setup;
		self->nextThink = level.time + 1;
	}
	self->use = Use_target_push;
}


//==============================================================================
//
//trigger_hurt
//
//==============================================================================

//QUAKED trigger_hurt (.5 .5 .5) ? START_OFF TOGGLE SILENT NO_PROTECTION SLOW KILL FALL
//Any player that touches this will be hurt by "dmg" points of damage
//-------- KEYS --------
//dmg : number of points of damage inflicted to player per "wait" time lapse (default 5 - integer values only).
//wait : wait time before hurting again (in seconds. Default 0.1)
//noise : sound to be played when inflicting damage
//notsingle : when set to 1, entity will not spawn in Single Player mode
//notfree : when set to 1, entity will not spawn in "Free for all" and "Tournament" modes.
//notduel : when set to 1, entity will not spawn in "Teamplay" and "CTF" modes. (jaltodo)
//notteam : when set to 1, entity will not spawn in "Teamplay" and "CTF" modes.
//notctf : when set to 1, entity will not spawn in "Teamplay" and "CTF" modes. (jaltodo)
//-------- SPAWNFLAGS --------
//START_OFF : needs to be triggered (toggle) for damage
//TOGGLE : toogle
//SILENT : supresses the sizzling sound while player is being hurt.
//NO_PROTECTION : player will be hurt regardless of protection (see Notes).
//SLOW : changes the damage rate to once per second.
//KILL : player will die instantly.
//FALL : player will die the next time he touches the ground.
//-------- NOTES --------
//The invulnerability power-up (item_enviro) does not protect the player from damage caused by this entity regardless of whether the NO_PROTECTION spawnflag is set or not. Triggering a trigger_hurt will have no effect if the START_OFF spawnflag is not set. A trigger_hurt always starts on in the game.

static void hurt_use( edict_t *self, edict_t *other, edict_t *activator )
{
	if( self->r.solid == SOLID_NOT )
		self->r.solid = SOLID_TRIGGER;
	else
		self->r.solid = SOLID_NOT;
	GClip_LinkEntity( self );

	if( !( self->spawnflags & 2 ) )
		self->use = NULL;
}

static void hurt_delayer_think( edict_t *self )
{
	edict_t *target = &game.edicts[self->s.ownerNum];
	float damage = target->health + (-GIB_HEALTH) + 1;

	if( target->r.client && target->r.client->resp.timeStamp == self->deathTimeStamp )
	{
		target->takedamage = qtrue;
		G_TakeDamage( target, target, world, vec3_origin, vec3_origin, target->s.origin, damage, 0, 0, DAMAGE_NO_PROTECTION, MOD_TRIGGER_HURT );
	}

	G_FreeEdict( self );
}

static void hurt_touch( edict_t *self, edict_t *other, cplane_t *plane, int surfFlags )
{
	int dflags;
	int damage;

	if( !other->takedamage || G_IsDead( other ) )
		return;

	if( self->s.team && self->s.team != other->s.team )
		return;

	if( G_TriggerWait( self, other ) )
		return;

	damage = self->dmg;

	if( self->spawnflags & 8 )
		dflags = DAMAGE_NO_PROTECTION;
	else
		dflags = 0;

	if( (self->spawnflags & 32) || (self->spawnflags & 64) ) // KILL, FALL
	{
		edict_t *delayer = G_Spawn();

		delayer->s.ownerNum = ENTNUM( other );
		delayer->think = hurt_delayer_think;
		delayer->nextThink = level.time + 1;
		delayer->deathTimeStamp = other->r.client->resp.timeStamp;
		
		// play the death sound, and delay the damage a little
		// so the entity is still in the transmission when the sound is sent
		if( self->noise_index )
		{
			G_Sound( other, CHAN_AUTO, self->noise_index, ATTN_NORM );
			other->pain_debounce_time = level.time;
		}

		// make it be dead so it doesn't touch the trigger again
		other->takedamage = qfalse;
		if( other->r.client )
			other->r.client->ps.pmove.stats[PM_STAT_NOUSERCONTROL] = level.time;

		return;
	}
	else if( !( self->spawnflags & 4 ) && self->noise_index )
	{
		if( (int)( level.time * 0.001 ) & 1 )
			G_Sound( other, CHAN_AUTO, self->noise_index, ATTN_NORM );
	}

	G_TakeDamage( other, self, world, vec3_origin, vec3_origin, other->s.origin, damage, damage, 0, dflags, MOD_TRIGGER_HURT );
}

void SP_trigger_hurt( edict_t *self )
{
	InitTrigger( self );

	if( self->dmg > 300 ) // HACK: force KILL spawnflag for big damages
		self->spawnflags |= 32;

	if( self->spawnflags & 4 ) // SILENT
	{   
		self->noise_index = 0;
	}
	else if( st.noise )
	{
		self->noise_index = trap_SoundIndex( st.noise );
		G_PureSound( st.noise );
	}
	else if( self->spawnflags & 32 || self->spawnflags & 64 ) // KILL or FALL
	{   
		self->noise_index = trap_SoundIndex( S_PLAYER_FALLDEATH );
	}
	else
	{
		self->noise_index = 0;
	}

	// gameteam field from editor
	if( st.gameteam >= TEAM_SPECTATOR && st.gameteam < GS_MAX_TEAMS )
	{
		self->s.team = st.gameteam;
	}
	else
	{
		self->s.team = TEAM_SPECTATOR;
	}

	self->touch = hurt_touch;

	if( !self->dmg )
		self->dmg = 5;

	if( self->spawnflags & 16 || !self->wait )
		self->wait = 0.1f;

	if( self->spawnflags & 1 )
		self->r.solid = SOLID_NOT;
	else
		self->r.solid = SOLID_TRIGGER;

	if( self->spawnflags & 2 )
		self->use = hurt_use;
}

//==============================================================================
//
//trigger_gravity
//
//==============================================================================

//QUAKED trigger_gravity (.5 .5 .5) ?
//Any player that touches this will change his gravity fraction. 1.0 is standard gravity
//-------- KEYS --------
//gravity : fraction of gravity to use. (Default 1.0)
//notsingle : when set to 1, entity will not spawn in Single Player mode
//notfree : when set to 1, entity will not spawn in "Free for all" and "Tournament" modes.
//notduel : when set to 1, entity will not spawn in "Teamplay" and "CTF" modes. (jaltodo)
//notteam : when set to 1, entity will not spawn in "Teamplay" and "CTF" modes.
//notctf : when set to 1, entity will not spawn in "Teamplay" and "CTF" modes. (jaltodo)
//-------- NOTES --------
//Changes the touching entites gravity to the value of "gravity".  1.0 is standard gravity for the level.
static void trigger_gravity_touch( edict_t *self, edict_t *other, cplane_t *plane, int surfFlags )
{
	if( self->s.team && self->s.team != other->s.team )
		return;

	other->gravity = self->gravity;
}

void SP_trigger_gravity( edict_t *self )
{
	if( st.gravity == 0 )
	{
		if( developer->integer )
			G_Printf( "trigger_gravity without gravity set at %s\n", vtos( self->s.origin ) );
		G_FreeEdict( self );
		return;
	}

	// gameteam field from editor
	if( st.gameteam >= TEAM_SPECTATOR && st.gameteam < GS_MAX_TEAMS )
	{
		self->s.team = st.gameteam;
	}
	else
	{
		self->s.team = TEAM_SPECTATOR;
	}

	InitTrigger( self );
	self->gravity = atof( st.gravity );
	self->touch = trigger_gravity_touch;
}


//QUAKED trigger_teleport (.5 .5 .5) ? SPECTATOR
//Players touching this will be teleported. Target it to a misc_teleporter_dest.
//-------- KEYS --------
//target : this points to the entity to activate.
//targetname : activating trigger points to this.
//noise : play this noise when triggered
//wait : time in seconds until trigger becomes re-triggerable after it's been touched (default 0.2, -1 = trigger once).
//notsingle : when set to 1, entity will not spawn in Single Player mode
//notfree : when set to 1, entity will not spawn in "Free for all" and "Tournament" modes.
//notduel : when set to 1, entity will not spawn in "Teamplay" and "CTF" modes. (jaltodo)
//notteam : when set to 1, entity will not spawn in "Teamplay" and "CTF" modes.
//notctf : when set to 1, entity will not spawn in "Teamplay" and "CTF" modes. (jaltodo)
//-------- SPAWNFLAGS --------
//SPECTATOR : &1 only teleport players moving in spectator mode
//-------- NOTES --------
//Target it to a misc_teleporter_dest.

static void old_teleporter_touch( edict_t *self, edict_t *other, cplane_t *plane, int surfFlags )
{
	edict_t	*dest;
	int i;
	vec3_t velocity, axis[3], angles;
	float speed;
	vec3_t org;

	if( !other->r.client )
		return;
	if( self->s.team && self->s.team != other->s.team )
		return;
	if( other->r.client->ps.pmove.pm_type > PM_SPECTATOR )
		return;
	if( self->spawnflags & 1 && other->r.client->ps.pmove.pm_type != PM_SPECTATOR )
		return;

	// match countdown
	if( GS_MatchState() == MATCH_STATE_COUNTDOWN )
		return;

	// wait delay
	if( self->timeStamp > level.time )
		return;

	self->timeStamp = level.time + ( self->wait * 1000 );

	dest = G_Find( NULL, FOFS( targetname ), self->target );
	if( !dest )
	{
		if( developer->integer )
			G_Printf( "Couldn't find destination.\n" );
		return;
	}

	if( self->s.modelindex )
	{
		org[0] = self->s.origin[0] + 0.5 * ( self->r.mins[0] + self->r.maxs[0] );
		org[1] = self->s.origin[1] + 0.5 * ( self->r.mins[1] + self->r.maxs[1] );
		org[2] = self->s.origin[2] + 0.5 * ( self->r.mins[2] + self->r.maxs[2] );
	}
	else
		VectorCopy( self->s.origin, org );

	// play custom sound if any (played from the teleporter entrance)
	if( self->noise_index )
		G_PositionedSound( org, CHAN_AUTO, self->noise_index, ATTN_NORM );

	// draw the teleport entering effect
	G_TeleportEffect( other, qfalse );

	//
	// teleport the player
	//

	VectorCopy( other->r.client->ps.pmove.velocity, velocity );

	velocity[2] = 0; // ignore vertical velocity
	speed = VectorLengthFast( velocity );

	// if someone enters a portal backwards, inverse the destination YAW angle
#if 0
	VectorCopy( other->s.angles, angles );
	angles[PITCH] = 0;
	AngleVectors( angles, axis[0], NULL, NULL );
	VectorSubtract( org, other->s.origin, org );

	VectorCopy( dest->s.angles, angles );
	if( DotProduct( org, axis[0] ) < 0 )
		angles[YAW] = anglemod( angles[YAW] - 180 );
#else
	VectorCopy( dest->s.angles, angles );
#endif

	AnglesToAxis( dest->s.angles, axis );
	VectorScale( axis[0], speed, other->r.client->ps.pmove.velocity );

	VectorCopy( angles, other->r.client->ps.viewangles );
	VectorCopy( dest->s.origin, other->r.client->ps.pmove.origin );

	// set the delta angle
	for( i = 0; i < 3; i++ )
		other->r.client->ps.pmove.delta_angles[i] = ANGLE2SHORT( other->r.client->ps.viewangles[i] ) - other->r.client->ucmd.angles[i];

	other->r.client->ps.pmove.pm_flags |= PMF_TIME_TELEPORT;
	other->s.teleported = qtrue;
	other->r.client->ps.pmove.pm_time = 1; // force the minimum no control delay

	// update the entity from the pmove
	VectorCopy( other->r.client->ps.viewangles, other->s.angles );
	VectorCopy( other->r.client->ps.pmove.origin, other->s.origin );
	VectorCopy( other->r.client->ps.pmove.origin, other->s.old_origin );
	VectorCopy( other->r.client->ps.pmove.origin, other->olds.origin );
	VectorCopy( other->r.client->ps.pmove.velocity, other->velocity );

	// unlink to make sure it can't possibly interfere with KillBox
	GClip_UnlinkEntity( other );

	// kill anything at the destination
	if( !KillBox( other ) )
	{
	}

	GClip_LinkEntity( other );

	// add the teleport effect at the destination
	G_TeleportEffect( other, qtrue );
}

void SP_trigger_teleport( edict_t *ent )
{
	if( !ent->target )
	{
		if( developer->integer )
			G_Printf( "teleporter without a target.\n" );
		G_FreeEdict( ent );
		return;
	}

	if( st.noise )
	{
		ent->noise_index = trap_SoundIndex( st.noise );
		G_PureSound( st.noise );
	}

	// gameteam field from editor
	if( st.gameteam >= TEAM_SPECTATOR && st.gameteam < GS_MAX_TEAMS )
	{
		ent->s.team = st.gameteam;
	}
	else
	{
		ent->s.team = TEAM_SPECTATOR;
	}

	InitTrigger( ent );
	ent->touch = old_teleporter_touch;
}

//QUAKED info_teleport_destination (0.5 0.5 0.5) (-16 -16 -24) (16 16 32)
//You can point trigger_teleports at these.
//-------- KEYS --------
//targetname : must match the target key of entity that uses this for pointing.
//notsingle : when set to 1, entity will not spawn in Single Player mode
//notfree : when set to 1, entity will not spawn in "Free for all" and "Tournament" modes.
//notduel : when set to 1, entity will not spawn in "Teamplay" and "CTF" modes. (jaltodo)
//notteam : when set to 1, entity will not spawn in "Teamplay" and "CTF" modes.
//notctf : when set to 1, entity will not spawn in "Teamplay" and "CTF" modes. (jaltodo)

void SP_info_teleport_destination( edict_t *ent )
{
	ent->s.origin[2] += 16;

	GS_SnapInitialPosition( ent->s.origin, playerbox_stand_mins, playerbox_stand_maxs, ent->s.number, MASK_PLAYERSOLID );
}
