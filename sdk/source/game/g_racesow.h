
// Functions

// alternative version of G_SplashFrac from g_combat.c
void rs_SplashFrac( const vec3_t origin, const vec3_t mins, const vec3_t maxs, const vec3_t point, float maxradius, vec3_t pushdir, float *kickFrac, float *dmgFrac );

// Triggers
void RS_UseShooter( edict_t *self, edict_t *other, edict_t *activator );
void RS_InitShooter( edict_t *self, int weapon );
void RS_InitShooter_Finish( edict_t *self );
void RS_shooter_rocket( edict_t *self );
void RS_shooter_plasma( edict_t *self );
void RS_shooter_grenade( edict_t *self );