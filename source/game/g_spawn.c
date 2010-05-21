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

const field_t fields[] = {
	{ "classname", FOFS( classname ), F_LSTRING },
	{ "origin", FOFS( s.origin ), F_VECTOR },
	{ "model", FOFS( model ), F_LSTRING },
	{ "model2", FOFS( model2 ), F_LSTRING },
	{ "spawnflags", FOFS( spawnflags ), F_INT },
	{ "speed", FOFS( speed ), F_FLOAT },
	{ "target", FOFS( target ), F_LSTRING },
	{ "targetname", FOFS( targetname ), F_LSTRING },
	{ "pathtarget", FOFS( pathtarget ), F_LSTRING },
	{ "killtarget", FOFS( killtarget ), F_LSTRING },
	{ "message", FOFS( message ), F_LSTRING },
	{ "team", FOFS( team ), F_LSTRING },
	{ "wait", FOFS( wait ), F_FLOAT },
	{ "delay", FOFS( delay ), F_FLOAT },
	{ "style", FOFS( style ), F_INT },
	{ "count", FOFS( count ), F_INT },
	{ "health", FOFS( health ), F_FLOAT },
	{ "sounds", FOFS( sounds ), F_LSTRING },
	{ "light", FOFS( light ), F_FLOAT },
	{ "color", FOFS( color ), F_VECTOR },
	{ "dmg", FOFS( dmg ), F_INT },
	{ "angles", FOFS( s.angles ), F_VECTOR },
	{ "mangle", FOFS( s.angles ), F_VECTOR },
	{ "angle", FOFS( s.angles ), F_ANGLEHACK },
	{ "mass", FOFS( mass ), F_INT },
	{ "attenuation", FOFS( attenuation ), F_FLOAT },
	{ "map", FOFS( map ), F_LSTRING },

	// temp spawn vars -- only valid when the spawn function is called
	{ "lip", STOFS( lip ), F_INT, FFL_SPAWNTEMP },
	{ "distance", STOFS( distance ), F_INT, FFL_SPAWNTEMP },
	{ "radius", STOFS( radius ), F_FLOAT, FFL_SPAWNTEMP },
	{ "roll", STOFS( roll ), F_FLOAT, FFL_SPAWNTEMP },
	{ "height", STOFS( height ), F_INT, FFL_SPAWNTEMP },
	{ "phase", STOFS( phase ), F_FLOAT, FFL_SPAWNTEMP },
	{ "noise", STOFS( noise ), F_LSTRING, FFL_SPAWNTEMP },
	{ "noise_start", STOFS( noise_start ), F_LSTRING, FFL_SPAWNTEMP },
	{ "noise_stop", STOFS( noise_stop ), F_LSTRING, FFL_SPAWNTEMP },
	{ "pausetime", STOFS( pausetime ), F_FLOAT, FFL_SPAWNTEMP },
	{ "item", STOFS( item ), F_LSTRING, FFL_SPAWNTEMP },
	{ "gravity", STOFS( gravity ), F_LSTRING, FFL_SPAWNTEMP },
	{ "music", STOFS( music ), F_LSTRING, FFL_SPAWNTEMP },
	{ "fov", STOFS( fov ), F_FLOAT, FFL_SPAWNTEMP },
	{ "nextmap", STOFS( nextmap ), F_LSTRING, FFL_SPAWNTEMP },
	{ "notsingle", STOFS( notsingle ), F_INT, FFL_SPAWNTEMP },
	{ "notteam", STOFS( notteam ), F_INT, FFL_SPAWNTEMP },
	{ "notfree", STOFS( notfree ), F_INT, FFL_SPAWNTEMP },
	{ "notduel", STOFS( notduel ), F_INT, FFL_SPAWNTEMP },
	{ "notctf", STOFS( notctf ), F_INT, FFL_SPAWNTEMP },
	{ "notffa", STOFS( notffa ), F_INT, FFL_SPAWNTEMP },
	{ "noents", STOFS( noents ), F_INT, FFL_SPAWNTEMP },
	{ "gameteam", STOFS( gameteam ), F_INT, FFL_SPAWNTEMP },
	{ "weight", STOFS( weight ), F_INT, FFL_SPAWNTEMP },
	{ "scale", STOFS( scale ), F_FLOAT, FFL_SPAWNTEMP },
	{ "gametype", STOFS( gametype ), F_LSTRING, FFL_SPAWNTEMP },
	{ "debris1", STOFS( debris1 ), F_LSTRING, FFL_SPAWNTEMP },
	{ "debris2", STOFS( debris2 ), F_LSTRING, FFL_SPAWNTEMP },

	{ NULL, 0, F_INT, 0 }
};


typedef struct
{
	char *name;
	void ( *spawn )( edict_t *ent );
} spawn_t;

static void SP_worldspawn( edict_t *ent );

spawn_t	spawns[] = {
	{ "info_player_start", SP_info_player_start },
	{ "info_player_deathmatch", SP_info_player_deathmatch },
	{ "info_player_intermission", SP_info_player_intermission },

	{ "func_plat", SP_func_plat },
	{ "func_button", SP_func_button },
	{ "func_door", SP_func_door },
	{ "func_door_rotating", SP_func_door_rotating },
	{ "func_door_secret", SP_func_door_secret },
	{ "func_water", SP_func_water },
	{ "func_rotating", SP_func_rotating },
	{ "func_train", SP_func_train },
	{ "func_conveyor", SP_func_conveyor },
	{ "func_wall", SP_func_wall },
	{ "func_object", SP_func_object },
	{ "func_explosive", SP_func_explosive },
	{ "func_killbox", SP_func_killbox },
	{ "func_static", SP_func_static },
	{ "func_bobbing", SP_func_bobbing },
	{ "func_pendulum", SP_func_pendulum },

	{ "trigger_always", SP_trigger_always },
	{ "trigger_once", SP_trigger_once },
	{ "trigger_multiple", SP_trigger_multiple },
	{ "trigger_relay", SP_trigger_relay },
	{ "trigger_push", SP_trigger_push },
	{ "trigger_hurt", SP_trigger_hurt },
	{ "trigger_counter", SP_trigger_counter },
	{ "trigger_elevator", SP_trigger_elevator },
	{ "trigger_gravity", SP_trigger_gravity },

	{ "target_temp_entity", SP_target_temp_entity },
	{ "target_speaker", SP_target_speaker },
	{ "target_explosion", SP_target_explosion },
	{ "target_spawner", SP_target_spawner },
	{ "target_crosslevel_trigger", SP_target_crosslevel_trigger },
	{ "target_crosslevel_target", SP_target_crosslevel_target },
	{ "target_laser", SP_target_laser },
	{ "target_lightramp", SP_target_lightramp },
	{ "target_string", SP_target_string },
	{ "target_location", SP_target_location },
	{ "target_position", SP_target_position },
	{ "target_print", SP_target_print },
	{ "target_give", SP_target_give },
	{ "target_push", SP_info_notnull },
	{ "target_changelevel", SP_target_changelevel },

	{ "worldspawn", SP_worldspawn },

	// Racesow
	{ "target_relay", RS_target_relay },
	{ "target_delay", RS_target_delay },
	{ "shooter_rocket", RS_shooter_rocket },
	{ "shooter_grenade", RS_shooter_grenade },
	{ "shooter_plasma", RS_shooter_plasma },
	// !Racesow

	{ "light", SP_light },
	{ "light_mine1", SP_light_mine },
	{ "info_null", SP_info_null },
	{ "func_group", SP_info_null },
	{ "info_notnull", SP_info_notnull },
	{ "info_camp", SP_info_camp },
	{ "path_corner", SP_path_corner },

	{ "misc_teleporter_dest", SP_misc_teleporter_dest },

	{ "trigger_teleport", SP_trigger_teleport },
	{ "info_teleport_destination", SP_info_teleport_destination },

	{ "misc_model", SP_misc_model },
	{ "misc_portal_surface", SP_misc_portal_surface },
	{ "misc_portal_camera", SP_misc_portal_camera },
	{ "misc_skyportal", SP_skyportal },
	{ "props_skyportal", SP_skyportal },

	{ NULL, NULL }
};

static gsitem_t *G_ItemForEntity( edict_t *ent )
{
	gsitem_t *item;

	// check item spawn functions
	if( ( item = GS_FindItemByClassname( ent->classname ) ) != NULL )
		return item;

	return NULL;
}

//=============
//G_CanSpawnEntity
//=============
static qboolean G_CanSpawnEntity( edict_t *ent )
{
	gsitem_t *item;

	if( ent == world )
		return qtrue;

	if( !Q_stricmp( gs.gametypeName, "dm" ) && st.notfree )
		return qfalse;
	if( !Q_stricmp( gs.gametypeName, "duel" ) && st.notduel )
		return qfalse;
	if( !Q_stricmp( gs.gametypeName, "tdm" ) && st.notteam )
		return qfalse;
	if( !Q_stricmp( gs.gametypeName, "ctf" ) && st.notctf )
		return qfalse;

	// check for multiplayer-disabled entities for Q1 maps
	if( G_IsQ1Map() )
	{
//#define SPAWNFLAG_NOT_DEATHMATCH	2048
		if( ent->spawnflags & 2048 )
			st.gametype = "sp coop"; // single-player, coop
	}

	// check for Q3TA-style inhibition key
	if( st.gametype )
	{
		if( !strstr( st.gametype, gs.gametypeName ) )
			return qfalse;
	}

	if( ( item = G_ItemForEntity( ent ) ) != NULL )
	{
		// not pickable items aren't either spawnable
		if( !( item->flags & ITFLAG_PICKABLE ) )
			return qfalse;

		if( !G_Gametype_CanSpawnItem( item ) )
			return qfalse;
	}

	return qtrue;
}

//===============
//G_CallSpawn
//
//Finds the spawn function for the entity and calls it
//===============
qboolean G_CallSpawn( edict_t *ent )
{
	spawn_t	*s;
	gsitem_t	*item;

	if( !ent->classname )
	{
		if( developer->integer )
			G_Printf( "G_CallSpawn: NULL classname\n" );
		return qfalse;
	}

	if( ( item = G_ItemForEntity( ent ) ) != NULL )
	{
		SpawnItem( ent, item );
		return qtrue;
	}

	// check normal spawn functions
	for( s = spawns; s->name; s++ )
	{
		if( !Q_stricmp( s->name, ent->classname ) )
		{
			s->spawn( ent );
			return qtrue;
		}
	}

	// see if there's a spawn definition in the gametype scripts
	if( G_asCallMapEntitySpawnScript( ent->classname, ent ) )
		return qtrue; // handled by the script

	if( sv_cheats->integer || developer->integer ) // mappers load their maps with devmap
		G_Printf( "%s doesn't have a spawn function\n", ent->classname );

	return qfalse;
}

//=============
//G_SpawnTempValue
//=============
char *G_SpawnTempValue( const char *key )
{
	const field_t *f;
	static int ss_offset = -1;
	static char value[MAX_TOKEN_CHARS];
	qbyte *b = (qbyte *)&st;

	value[0] = '\0';

	f = fields;
	if( ss_offset >= 0 )
		f += ss_offset;

	for( ; f->name; f++ )
	{
		// found it
		if( f->flags & FFL_SPAWNTEMP )
		{
			if( ss_offset < 0 )
				ss_offset = f - fields;
		}
		else
		{
			continue;
		}

		if( !Q_stricmp( f->name, key ) )
		{
			switch( f->type )
			{
			case F_LSTRING:
				Q_strncpyz( value, *(char **)( b+f->ofs ), sizeof( value ) );
				break;
			case F_VECTOR:
				Q_snprintfz( value, sizeof( value ), "%f %f %f", ( (float *)( b+f->ofs ) )[0], ( (float *)( b+f->ofs ) )[1], ( (float *)( b+f->ofs ) )[2] );
				break;
			case F_INT:
				Q_snprintfz( value, sizeof( value ), "%i", *(int *)( b+f->ofs ) );
				break;
			case F_FLOAT:
				Q_snprintfz( value, sizeof( value ), "%f", *(float *)( b+f->ofs ) );
				break;
			case F_ANGLEHACK:
				Q_snprintfz( value, sizeof( value ), "0 %f 0", ( (float *)( b+f->ofs ) )[1] );
				break;
			default:
				break; // FIXME: Should this be error?
			}

			return value;
		}
	}

	if( developer->integer )
		G_Printf( "%s is not a field\n", key );

	return value;
}

//=============
//ED_NewString
//=============
static char *ED_NewString( const char *string )
{
	char *newb, *new_p;
	size_t i, l;

	l = strlen( string ) + 1;
	newb = &level.map_parsed_ents[level.map_parsed_len];
	level.map_parsed_len += l;

	new_p = newb;

	for( i = 0; i < l; i++ )
	{
		if( string[i] == '\\' && i < l-1 )
		{
			i++;
			if( string[i] == 'n' )
			{
				*new_p++ = '\n';
			}
			else
			{
				*new_p++ = '/';
				*new_p++ = string[i];
			}
		}
		else
			*new_p++ = string[i];
	}

	*new_p = '\0';
	return newb;
}

//===============
//ED_ParseField
//
//Takes a key/value pair and sets the binary values
//in an edict
//===============
static void ED_ParseField( char *key, char *value, edict_t *ent )
{
	const field_t *f;
	qbyte *b;
	float v;
	vec3_t vec;

	for( f = fields; f->name; f++ )
	{
		if( !Q_stricmp( f->name, key ) )
		{ // found it
			if( f->flags & FFL_SPAWNTEMP )
				b = (qbyte *)&st;
			else
				b = (qbyte *)ent;

			switch( f->type )
			{
			case F_LSTRING:
				*(char **)( b+f->ofs ) = ED_NewString( value );
				break;
			case F_VECTOR:
				sscanf( value, "%f %f %f", &vec[0], &vec[1], &vec[2] );
				( (float *)( b+f->ofs ) )[0] = vec[0];
				( (float *)( b+f->ofs ) )[1] = vec[1];
				( (float *)( b+f->ofs ) )[2] = vec[2];
				break;
			case F_INT:
				*(int *)( b+f->ofs ) = atoi( value );
				break;
			case F_FLOAT:
				*(float *)( b+f->ofs ) = atof( value );
				break;
			case F_ANGLEHACK:
				v = atof( value );
				( (float *)( b+f->ofs ) )[0] = 0;
				( (float *)( b+f->ofs ) )[1] = v;
				( (float *)( b+f->ofs ) )[2] = 0;
				break;
			case F_IGNORE:
				break;
			default:
				break; // FIXME: Should this be error?
			}
			return;
		}
	}

	if( developer->integer )
		G_Printf( "%s is not a field\n", key );
}

//====================
//ED_ParseEdict
//
//Parses an edict out of the given string, returning the new position
//ed should be a properly initialized empty edict.
//====================
static char *ED_ParseEdict( char *data, edict_t *ent )
{
	qboolean init;
	char keyname[256];
	char *com_token;

	init = qfalse;
	memset( &st, 0, sizeof( st ) );

	// go through all the dictionary pairs
	while( 1 )
	{
		// parse key
		com_token = COM_Parse( &data );
		if( com_token[0] == '}' )
			break;
		if( !data )
			G_Error( "ED_ParseEntity: EOF without closing brace" );

		Q_strncpyz( keyname, com_token, sizeof( keyname ) );

		// parse value
		com_token = COM_Parse( &data );
		if( !data )
			G_Error( "ED_ParseEntity: EOF without closing brace" );

		if( com_token[0] == '}' )
			G_Error( "ED_ParseEntity: closing brace without data" );

		init = qtrue;

		// keynames with a leading underscore are used for utility comments,
		// and are immediately discarded by quake
		if( keyname[0] == '_' )
			continue;

		ED_ParseField( keyname, com_token, ent );
	}

	if( !init )
		memset( ent, 0, sizeof( *ent ) );

	return data;
}

//================
//G_FindTeams
//
//Chain together all entities with a matching team field.
//
//All but the first will have the FL_TEAMSLAVE flag set.
//All but the last will have the teamchain field set to the next one
//================
static void G_FindTeams( void )
{
	edict_t	*e, *e2, *chain;
	int i, j;
	int c, c2;

	// for Q1 maps assign touching doors to the same team
	if( G_IsQ1Map() )
	{
		for( i = 1, e = game.edicts+i; i < game.numentities; i++, e++ )
		{
			if( !G_EntIsADoor( e ) )
				continue;
			if( e->team )
				continue; // already teamed

			// find touching doors
			for( j = i+1, e2 = e+1; j < game.numentities; j++, e2++ )
			{
				if( !G_EntIsADoor( e2 ) )
					continue;
				if( e2->team )
					continue; // already teamed
				if( strcmp( e->classname, e2->classname ) )
					continue;

				if( !BoundsIntersect( e->r.mins, e->r.maxs, e2->r.mins, e2->r.maxs ) )
					continue;

				if( !e->team )
					e->team = e->model;
				e2->team = e->team;
			}
		}
	}

	c = 0;
	c2 = 0;
	for( i = 1, e = game.edicts+i; i < game.numentities; i++, e++ )
	{
		if( !e->r.inuse )
			continue;
		if( !e->team )
			continue;
		if( e->flags & FL_TEAMSLAVE )
			continue;
		chain = e;
		e->teammaster = e;
		c++;
		c2++;
		for( j = i+1, e2 = e+1; j < game.numentities; j++, e2++ )
		{
			if( !e2->r.inuse )
				continue;
			if( !e2->team )
				continue;
			if( e2->flags & FL_TEAMSLAVE )
				continue;
			if( !strcmp( e->team, e2->team ) )
			{
				c2++;
				chain->teamchain = e2;
				e2->teammaster = e;
				chain = e2;
				e2->flags |= FL_TEAMSLAVE;
			}
		}
	}

	if( developer->integer )
		G_Printf( "%i teams with %i entities\n", c, c2 );
}

void G_PrecacheMedia( void )
{
	//
	// MODELS
	//

	// THIS ORDER MUST MATCH THE DEFINES IN gs_public.h
	// you can add more, max 255

	trap_ModelIndex( "#gunblade/gunblade.md3" );      // WEAP_GUNBLADE
	trap_ModelIndex( "#machinegun/machinegun.md3" );    // WEAP_MACHINEGUN
	trap_ModelIndex( "#riotgun/riotgun.md3" );        // WEAP_RIOTGUN
	trap_ModelIndex( "#glauncher/glauncher.md3" );    // WEAP_GRENADELAUNCHER
	trap_ModelIndex( "#rlauncher/rlauncher.md3" );    // WEAP_ROCKETLAUNCHER
	trap_ModelIndex( "#plasmagun/plasmagun.md3" );    // WEAP_PLASMAGUN
	trap_ModelIndex( "#lasergun/lasergun.md3" );      // WEAP_LASERGUN
	trap_ModelIndex( "#electrobolt/electrobolt.md3" ); // WEAP_ELECTROBOLT
	trap_ModelIndex( "#instagun/instagun.md3" );      // WEAP_INSTAGUN

	//-------------------

	// precache our basic player models, they are just a very few
	trap_ModelIndex( "$models/players/bigvic" );
	trap_ModelIndex( "$models/players/monada" );
	trap_ModelIndex( "$models/players/silverclaw" );
	trap_ModelIndex( "$models/players/padpork" );
	trap_ModelIndex( "$models/players/bobot" );
	trap_ModelIndex( "$models/players/viciious" );
	//trap_ModelIndex( "$models/players/budndumby" );

	trap_SkinIndex( "models/players/bigvic/default" );
	trap_SkinIndex( "models/players/monada/default" );
	trap_SkinIndex( "models/players/silverclaw/default" );
	trap_SkinIndex( "models/players/padpork/default" );
	trap_SkinIndex( "models/players/bobot/default" );
	trap_SkinIndex( "models/players/viciious/default" );
	//trap_SkinIndex( "models/players/budndumby/default" );

	// FIXME: Temporarily use normal gib until the head is fixed
	trap_ModelIndex( "models/objects/gibs/gib1/gib1.md3" );

	//
	// SOUNDS
	//

	// jalfixme : most of these sounds can be played from the clients

	trap_SoundIndex( S_WORLD_WATER_IN );    // feet hitting water
	trap_SoundIndex( S_WORLD_WATER_OUT );       // feet leaving water
	trap_SoundIndex( S_WORLD_UNDERWATER );

	trap_SoundIndex( S_WORLD_SLIME_IN );
	trap_SoundIndex( S_WORLD_SLIME_OUT );
	trap_SoundIndex( S_WORLD_UNDERSLIME );

	trap_SoundIndex( S_WORLD_LAVA_IN );
	trap_SoundIndex( S_WORLD_LAVA_OUT );
	trap_SoundIndex( S_WORLD_UNDERLAVA );

	trap_SoundIndex( va( S_PLAYER_BURN_1_to_2, 1 ) );
	trap_SoundIndex( va( S_PLAYER_BURN_1_to_2, 2 ) );

	//wsw: pb disable unreferenced sounds
	//trap_SoundIndex (S_LAND);				// landing thud
	trap_SoundIndex( S_HIT_WATER );

	trap_SoundIndex( S_WEAPON_NOAMMO );

	// announcer

	// readyup
	trap_SoundIndex( S_ANNOUNCER_READY_UP_POLITE );
	trap_SoundIndex( S_ANNOUNCER_READY_UP_PISSEDOFF );

	// countdown
	trap_SoundIndex( va( S_ANNOUNCER_COUNTDOWN_GET_READY_TO_FIGHT_1_to_2, 1 ) );
	trap_SoundIndex( va( S_ANNOUNCER_COUNTDOWN_GET_READY_TO_FIGHT_1_to_2, 2 ) );
	trap_SoundIndex( va( S_ANNOUNCER_COUNTDOWN_READY_1_to_2, 1 ) );
	trap_SoundIndex( va( S_ANNOUNCER_COUNTDOWN_READY_1_to_2, 2 ) );
	trap_SoundIndex( va( S_ANNOUNCER_COUNTDOWN_COUNT_1_to_3_SET_1_to_2, 1, 1 ) );
	trap_SoundIndex( va( S_ANNOUNCER_COUNTDOWN_COUNT_1_to_3_SET_1_to_2, 2, 1 ) );
	trap_SoundIndex( va( S_ANNOUNCER_COUNTDOWN_COUNT_1_to_3_SET_1_to_2, 3, 1 ) );
	trap_SoundIndex( va( S_ANNOUNCER_COUNTDOWN_COUNT_1_to_3_SET_1_to_2, 1, 2 ) );
	trap_SoundIndex( va( S_ANNOUNCER_COUNTDOWN_COUNT_1_to_3_SET_1_to_2, 2, 2 ) );
	trap_SoundIndex( va( S_ANNOUNCER_COUNTDOWN_COUNT_1_to_3_SET_1_to_2, 3, 2 ) );
	trap_SoundIndex( va( S_ANNOUNCER_COUNTDOWN_FIGHT_1_to_2, 1 ) );
	trap_SoundIndex( va( S_ANNOUNCER_COUNTDOWN_FIGHT_1_to_2, 2 ) );

	// postmatch
	trap_SoundIndex( va( S_ANNOUNCER_POSTMATCH_GAMEOVER_1_to_2, 1 ) );
	trap_SoundIndex( va( S_ANNOUNCER_POSTMATCH_GAMEOVER_1_to_2, 2 ) );

	// timeout
	trap_SoundIndex( va( S_ANNOUNCER_TIMEOUT_MATCH_RESUMED_1_to_2, 1 ) );
	trap_SoundIndex( va( S_ANNOUNCER_TIMEOUT_MATCH_RESUMED_1_to_2, 2 ) );
	trap_SoundIndex( va( S_ANNOUNCER_TIMEOUT_TIMEOUT_1_to_2, 1 ) );
	trap_SoundIndex( va( S_ANNOUNCER_TIMEOUT_TIMEOUT_1_to_2, 2 ) );
	trap_SoundIndex( va( S_ANNOUNCER_TIMEOUT_TIMEIN_1_to_2, 1 ) );
	trap_SoundIndex( va( S_ANNOUNCER_TIMEOUT_TIMEIN_1_to_2, 2 ) );

	// callvote
	trap_SoundIndex( va( S_ANNOUNCER_CALLVOTE_CALLED_1_to_2, 1 ) );
	trap_SoundIndex( va( S_ANNOUNCER_CALLVOTE_CALLED_1_to_2, 2 ) );
	trap_SoundIndex( va( S_ANNOUNCER_CALLVOTE_FAILED_1_to_2, 1 ) );
	trap_SoundIndex( va( S_ANNOUNCER_CALLVOTE_FAILED_1_to_2, 2 ) );
	trap_SoundIndex( va( S_ANNOUNCER_CALLVOTE_PASSED_1_to_2, 1 ) );
	trap_SoundIndex( va( S_ANNOUNCER_CALLVOTE_PASSED_1_to_2, 2 ) );
	trap_SoundIndex( S_ANNOUNCER_CALLVOTE_VOTE_NOW );

	// overtime
	trap_SoundIndex( S_ANNOUNCER_OVERTIME_GOING_TO_OVERTIME );
	trap_SoundIndex( S_ANNOUNCER_OVERTIME_OVERTIME );
	trap_SoundIndex( va( S_ANNOUNCER_OVERTIME_SUDDENDEATH_1_to_2, 1 ) );
	trap_SoundIndex( va( S_ANNOUNCER_OVERTIME_SUDDENDEATH_1_to_2, 2 ) );

	// score
	trap_SoundIndex( va( S_ANNOUNCER_SCORE_TAKEN_LEAD_1_to_2, 1 ) );
	trap_SoundIndex( va( S_ANNOUNCER_SCORE_TAKEN_LEAD_1_to_2, 2 ) );
	trap_SoundIndex( va( S_ANNOUNCER_SCORE_LOST_LEAD_1_to_2, 1 ) );
	trap_SoundIndex( va( S_ANNOUNCER_SCORE_LOST_LEAD_1_to_2, 2 ) );
	trap_SoundIndex( va( S_ANNOUNCER_SCORE_TIED_LEAD_1_to_2, 1 ) );
	trap_SoundIndex( va( S_ANNOUNCER_SCORE_TIED_LEAD_1_to_2, 2 ) );

	if( GS_TeamBasedGametype() )
	{
		trap_SoundIndex( va( S_ANNOUNCER_SCORE_TEAM_TAKEN_LEAD_1_to_2, 1 ) );
		trap_SoundIndex( va( S_ANNOUNCER_SCORE_TEAM_TAKEN_LEAD_1_to_2, 2 ) );
		trap_SoundIndex( va( S_ANNOUNCER_SCORE_TEAM_LOST_LEAD_1_to_2, 1 ) );
		trap_SoundIndex( va( S_ANNOUNCER_SCORE_TEAM_LOST_LEAD_1_to_2, 2 ) );
		trap_SoundIndex( va( S_ANNOUNCER_SCORE_TEAM_TIED_LEAD_1_to_2, 1 ) );
		trap_SoundIndex( va( S_ANNOUNCER_SCORE_TEAM_TIED_LEAD_1_to_2, 2 ) );
		trap_SoundIndex( va( S_ANNOUNCER_SCORE_TEAM_TIED_LEAD_1_to_2, 1 ) );
		trap_SoundIndex( va( S_ANNOUNCER_SCORE_TEAM_TIED_LEAD_1_to_2, 2 ) );
		//trap_SoundIndex( va( S_ANNOUNCER_SCORE_TEAM_1_to_4_TAKEN_LEAD_1_to_2, 3, 1 ) );
		//trap_SoundIndex( va( S_ANNOUNCER_SCORE_TEAM_1_to_4_TAKEN_LEAD_1_to_2, 3, 2 ) );
		//trap_SoundIndex( va( S_ANNOUNCER_SCORE_TEAM_1_to_4_TAKEN_LEAD_1_to_2, 4, 1 ) );
		//trap_SoundIndex( va( S_ANNOUNCER_SCORE_TEAM_1_to_4_TAKEN_LEAD_1_to_2, 4, 2 ) );
	}

	//
	// LIGHTSTYLES
	//

	// light animation tables. 'a' is total darkness, 'z' is doublebright.

	// 0 normal
	trap_ConfigString( CS_LIGHTS+0, "m" );

	// 1 FLICKER (first variety)
	trap_ConfigString( CS_LIGHTS+1, "mmnmmommommnonmmonqnmmo" );

	// 2 SLOW STRONG PULSE
	trap_ConfigString( CS_LIGHTS+2, "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba" );

	// 3 CANDLE (first variety)
	trap_ConfigString( CS_LIGHTS+3, "mmmmmaaaaammmmmaaaaaabcdefgabcdefg" );

	// 4 FAST STROBE
	trap_ConfigString( CS_LIGHTS+4, "mamamamamama" );

	// 5 GENTLE PULSE 1
	trap_ConfigString( CS_LIGHTS+5, "jklmnopqrstuvwxyzyxwvutsrqponmlkj" );

	// 6 FLICKER (second variety)
	trap_ConfigString( CS_LIGHTS+6, "nmonqnmomnmomomno" );

	// 7 CANDLE (second variety)
	trap_ConfigString( CS_LIGHTS+7, "mmmaaaabcdefgmmmmaaaammmaamm" );

	// 8 CANDLE (third variety)
	trap_ConfigString( CS_LIGHTS+8, "mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa" );

	// 9 SLOW STROBE (fourth variety)
	trap_ConfigString( CS_LIGHTS+9, "aaaaaaaazzzzzzzz" );

	// 10 FLUORESCENT FLICKER
	trap_ConfigString( CS_LIGHTS+10, "mmamammmmammamamaaamammma" );

	// 11 SLOW PULSE NOT FADE TO BLACK
	trap_ConfigString( CS_LIGHTS+11, "abcdefghijklmnopqrrqponmlkjihgfedcba" );

	// styles 32-62 are assigned by the light program for switchable lights

	// 63 testing
	trap_ConfigString( CS_LIGHTS+63, "a" );
}

//==============
//SpawnEntities
//
//Creates a server's entity / program execution context by
//parsing textual entity definitions out of an ent file.
//==============
void G_InitLevel( char *mapname, char *entities, int entstrlen, unsigned int levelTime, unsigned int serverTime, unsigned int realTime )
{
	char *mapString = NULL;
	char name[MAX_CONFIGSTRING_CHARS];
	int i;
	edict_t *ent;
	char *token;
	gsitem_t *item;
	// racesow
	int racesow_freestyle_maps_bug;
	// !racesow

	G_asGarbageCollect( qtrue );

	G_asCallShutdownScript();

	G_asShutdownGametypeScript();

	G_FreeCallvotes();

	game.serverTime = serverTime;
	game.realtime = realTime;
	game.levelSpawnCount++;

	GClip_ClearWorld(); // clear areas links

	if( !entities )
		G_Error( "G_SpawnLevel: NULL entities string\n" );

	// make a copy of the raw entities string so it's not freed with the pool
	mapString = G_Malloc( entstrlen + 1 );
	memcpy( mapString, entities, entstrlen );
	Q_strncpyz( name, mapname, sizeof( name ) );

	// clear old data

	G_LevelInitPool( strlen( mapname ) + 1 + ( entstrlen + 1 ) * 2 + G_LEVELPOOL_BASE_SIZE );

	G_StringPoolInit();

	memset( &level, 0, sizeof( level_locals_t ) );
	memset( &gs.gameState, 0, sizeof( gs.gameState ) );

	level.spawnedTimeStamp = game.realtime;
	level.time = levelTime;

	// get the strings back
	Q_strncpyz( level.mapname, name, sizeof( level.mapname ) );
	level.mapString = G_LevelMalloc( entstrlen + 1 );
	level.mapStrlen = entstrlen;
	memcpy( level.mapString, mapString, entstrlen );
	G_Free( mapString );
	mapString = NULL;

	// make a copy of the raw entities string for parsing
	level.map_parsed_ents = G_LevelMalloc( entstrlen + 1 );
	level.map_parsed_ents[0] = 0;

	if( !level.time )
		memset( game.edicts, 0, game.maxentities * sizeof( game.edicts[0] ) );
	else
	{
		G_FreeEdict( world );
		for( i = gs.maxclients + 1; i < game.maxentities; i++ )
		{
			if( game.edicts[i].r.inuse )
				G_FreeEdict( game.edicts + i );
		}
	}

	game.numentities = gs.maxclients + 1;

	// link client fields on player ents
	for( i = 0; i < gs.maxclients; i++ )
	{
		game.edicts[i+1].s.number = i+1;
		game.edicts[i+1].r.client = &game.clients[i];
		game.edicts[i+1].r.inuse = ( trap_GetClientState( i ) >= CS_CONNECTED );
		memset( &game.clients[i].level, 0, sizeof( game.clients[0].level ) );
		game.clients[i].level.timeStamp = level.time;
	}

	// initialize game subsystems
	trap_ConfigString( CS_MAPNAME, level.mapname );
	trap_ConfigString( CS_SKYBOX, "" );
	trap_ConfigString( CS_AUDIOTRACK, "" );
	trap_ConfigString( CS_SCORESTATNUMS, va( "%i", STAT_SCORE ) );
	trap_ConfigString( CS_POWERUPEFFECTS, va( "%i %i %i", EF_QUAD, EF_SHELL, EF_CARRIER ) );
	trap_ConfigString( CS_SCB_PLAYERTAB_LAYOUT, "" );
	trap_ConfigString( CS_SCB_PLAYERTAB_TITLES, "" );
	trap_ConfigString( CS_MATCHNAME, "" );

	G_InitGameCommands();
	G_MapLocations_Init();
	G_CallVotes_Init();
	G_SpawnQueue_Init();
	G_Teams_Init();
	G_Gametype_Init();
	G_PrecacheItems(); // set configstrings for items (gametype must be initialized)
	G_PrecacheMedia();
	G_PrecacheGameCommands(); // adding commands after this point won't update them to the client
	AI_InitLevel(); // load navigation file of the current map

	// start spawning entities

	level.canSpawnEntities = qtrue;
	G_InitBodyQueue(); // reserve some spots for dead player bodies

	entities = level.mapString;

	racesow_freestyle_maps_bug=trap_Cvar_Get( "g_freestyle", "0", CVAR_SERVERINFO|CVAR_ARCHIVE|CVAR_NOSET )->integer;

	i = 0;
	ent = NULL;
	while( 1 )
	{
		// parse the opening brace
		token = COM_Parse( &entities );
		if( !entities )
			break;
		if( token[0] != '{' )
			G_Error( "G_SpawnMapEntities: found %s when expecting {", token );

		if( !ent )
		{
			ent = world;
			G_InitEdict( world );
		}
		else
			ent = G_Spawn();

		entities = ED_ParseEdict( entities, ent );
		if( !ent->classname )
		{
			i++;
			G_FreeEdict( ent );
			continue;
		}

		if( !G_CanSpawnEntity( ent ) )
		{
			i++;
			G_FreeEdict( ent );
			continue;
		}

		if( !G_CallSpawn( ent ) )
		{
			i++;
			// racesow: introducing that bug again in order to make some freestyle map work
			if (!racesow_freestyle_maps_bug)
				G_FreeEdict( ent );
			// !racesow
			continue;
		}

		// check whether an item is allowed to spawn
		if( ( item = ent->item ) )
		{
			// not pickable items aren't spawnable
			if( item->flags & ITFLAG_PICKABLE )
			{
				if( G_Gametype_CanSpawnItem( item ) )
				{
					// override entity's classname with whatever item specifies
					ent->classname = item->classname;
					PrecacheItem( item );
					continue;
				}
			}

			i++;
			G_FreeEdict( ent );
			continue;
		}
	}

	G_FindTeams();

	// is the parsing string sane?
	assert( (int)level.map_parsed_len < entstrlen );
	level.map_parsed_ents[level.map_parsed_len] = 0;

	// make sure server got the edicts data
	trap_LocateEntities( game.edicts, sizeof( game.edicts[0] ), game.numentities, game.maxentities );

	// items need brush model entities spawned before they are linked
	G_Items_FinishSpawningItems();

	//
	// initialize game subsystems which require entities initialized
	//

	// call gametype specific
	if( level.gametype.asEngineHandle >= 0 )
		G_asCallLevelSpawnScript();

	AI_InitEntitiesData();

	// always start in warmup match state and let the thinking code
	// revert it to wait state if empty ( so gametype based item masks are setup )
	G_Match_LaunchState( MATCH_STATE_WARMUP );

	G_asGarbageCollect( qtrue );
}

qboolean G_RespawnLevel( void )
{
	G_InitLevel( level.mapname, level.mapString, level.mapStrlen, level.time, game.serverTime, game.realtime );
	return qtrue;
}

//===================================================================

//QUAKED worldspawn (0 0 0) ?
//
//Only used for the world.
//"sky"	environment map name
//"skyaxis"	vector axis for rotating sky
//"skyrotate"	speed of rotation in degrees/second
//"sounds"	music cd track number
//"gravity"	800 is default gravity
//"message"	text to print at user logon
//========================
static void SP_worldspawn( edict_t *ent )
{
	ent->movetype = MOVETYPE_PUSH;
	ent->r.solid = SOLID_YES;
	ent->r.inuse = qtrue;       // since the world doesn't use G_Spawn()
	VectorClear( ent->s.origin );
	VectorClear( ent->s.angles );
	GClip_SetBrushModel( ent, "*0" ); // sets mins / maxs and modelindex 1
	G_PureModel( "*0" );

	if( st.nextmap )
		Q_strncpyz( level.nextmap, st.nextmap, sizeof( level.nextmap ) );

	// make some data visible to the server
	/*message = trap_GetFullnameFromMapList( level.mapname );
	   if( message && message[0] )
	    ent->message = G_LevelCopyString( message );*/

	if( ent->message && ent->message[0] )
	{
		trap_ConfigString( CS_MESSAGE, ent->message );
		Q_strncpyz( level.level_name, ent->message, sizeof( level.level_name ) );
	}
	else
	{
		trap_ConfigString( CS_MESSAGE, level.mapname );
		Q_strncpyz( level.level_name, level.mapname, sizeof( level.level_name ) );
	}

	// send music
	if( st.music )
	{
		trap_ConfigString( CS_AUDIOTRACK, st.music );
		trap_PureSound( st.music );
	}

	if( st.gravity )
		trap_Cvar_Set( "g_gravity", st.gravity );
}
