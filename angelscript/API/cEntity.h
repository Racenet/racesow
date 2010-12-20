/**
 * cEntity
 */
class cEntity
{
public:
	/* object properties */
	cClient @ client;
	cItem @ item;
	cEntity @ groundEntity;
	cEntity @ owner;
	cEntity @ enemy;
	cEntity @ activator;
	int type;
	int modelindex;
	int modelindex2;
	int frame;
	int ownerNum;
	int counterNum;
	int skinNum;
	int colorRGBA;
	int weapon;
	bool teleported;
	uint effects;
	int sound;
	int team;
	int light;
	const bool inuse;
	uint svflags;
	int solid;
	int clipMask;
	int spawnFlags;
	int style;
	int moveType;
	uint nextThink;
	float health;
	int maxHealth;
	int viewHeight;
	int takeDamage;
	int damage;
	int projectileMaxDamage;
	int projectileMinDamage;
	int projectileMaxKnockback;
	int projectileMinKnockback;
	float projectileSplashRadius;
	int count;
	float wait;
	float delay;
	int waterLevel;
	float attenuation;
	int mass;
	uint timeStamp;
	int particlesSpeed;
	int particlesShaderIndex;
	int particlesSpread;
	int particlesSize;
	int particlesTime;
	int particlesFrequency;
	bool particlesSpherical;
	bool particlesBounce;
	bool particlesGravity;
	bool particlesExpandEffect;
	bool particlesShrinkEffect;

	/* object behaviors */
	cEntity @ f(); /* factory */ 

	/* global behaviors */

	/* object methods */
	cVec3 @ getVelocity();
	void setVelocity(cVec3 &in);
	cVec3 @ getAVelocity();
	void setAVelocity(cVec3 &in);
	cVec3 @ getOrigin();
	void setOrigin(cVec3 &in);
	cVec3 @ getOrigin2();
	void setOrigin2(cVec3 &in);
	cVec3 @ getAngles();
	void setAngles(cVec3 &in);
	void getSize(cVec3 @+mins, cVec3 @+maxs);
	void setSize(cVec3 @+mins, cVec3 @+maxs);
	void getMovedir(cVec3 &);
	void setMovedir();
	bool isBrushModel();
	void freeEntity();
	void linkEntity();
	void unlinkEntity();
	bool isGhosting();
	int entNum();
	int playerNum();
	cString @ getModelString();
	cString @ getModel2String();
	cString @ getSoundString();
	cString @ getClassname();
	cString @ getClassName();
	cString @ getTargetnameString();
	cString @ getTargetString();
	cString @ getMapString();
	void setTargetString( cString &in );
	void setTargetnameString( cString &in );
	void setClassname( cString &in );
	void setClassName( cString &in );
	void setMapString( cString &in );
	void ghost();
	void spawnqueueAdd();
	void teleportEffect( bool );
	void respawnEffect();
	void setupModel( cString &in );
	void setupModel( cString &in, cString &in );
	cEntity @ findTargetEntity( cEntity @from );
	cEntity @ findTargetingEntity( cEntity @from );
	void use( cEntity @targeter, cEntity @activator );
	void useTargets( cEntity @activator );
	cEntity @ dropItem( int tag );
	void addAIGoal( bool customReach );
	void addAIGoal();
	void removeAIGoal();
	void reachedAIGoal();
	void takeDamage( cEntity @inflicter, cEntity @attacker, cVec3 @dir, float damage, float knockback, float stun, int mod );
	void splashDamage( cEntity @attacker, int radius, float damage, float knockback, float stun, int mod );
	void explosionEffect( int radius );
};

