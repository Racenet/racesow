/* funcdefs */
funcdef void entThink(cEntity @ent);
funcdef void entTouch(cEntity @ent, cEntity @other, const Vec3 planeNormal, int surfFlags);
funcdef void entUse(cEntity @ent, cEntity @other, cEntity @activator);
funcdef void entPain(cEntity @ent, cEntity @other, float kick, float damage);
funcdef void entDie(cEntity @ent, cEntity @inflicter, cEntity @attacker);
funcdef void entStop(cEntity @ent);

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
	entThink @ think;
	entTouch @ touch;
	entUse @ use;
	entPain @ pain;
	entDie @ die;
	entStop @ stop;
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

	/* object methods */
	Vec3 getVelocity() const;
	void setVelocity(const Vec3 &in);
	Vec3 getAVelocity() const;
	void setAVelocity(const Vec3 &in);
	Vec3 getOrigin() const;
	void setOrigin(const Vec3 &in);
	Vec3 getOrigin2() const;
	void setOrigin2(const Vec3 &in);
	Vec3 getAngles() const;
	void setAngles(const Vec3 &in);
//racesow -- these methods were changed in racesow 0.6
////	void getSize(cVec3 @+, cVec3 @+);
////	void setSize(cVec3 @+, cVec3 @+);
//	void getSize(cVec3 @+mins, cVec3 @+maxs);
//	void setSize(cVec3 @+mins, cVec3 @+maxs);
//!racesow
	void getSize(Vec3 &out, Vec3 &out);
	void setSize(const Vec3 &in, const Vec3 &in);
	Vec3 getMovedir() const;
	void setMovedir();
	bool isBrushModel() const;
	void freeEntity();
	void linkEntity();
	void unlinkEntity();
	bool isGhosting() const;
	int entNum() const;
	int playerNum() const;
	String @ getModelString() const;
	String @ getModel2String() const;
	String @ getSoundString() const;
	String @ getClassname() const;
	String @ getClassName() const;
	String @ getTargetnameString() const;
	String @ getTargetString() const;
	String @ getMapString() const;
	void setTargetString( const String &in );
	void setTargetnameString( const String &in );
	void setClassname( const String &in );
	void setClassName( const String &in );
	void setMapString( const String &in );
	void ghost();
	void spawnqueueAdd();
	void teleportEffect( bool );
	void respawnEffect();
	void setupModel( const String &in );
	void setupModel( const String &in, const String &in );
	cEntity @ findTargetEntity( const cEntity @from ) const;
	cEntity @ findTargetingEntity( const cEntity @from ) const;
	void useTargets( const cEntity @activator );
	cEntity @ dropItem( int tag ) const;
	void addAIGoal( bool customReach );
	void addAIGoal();
	void removeAIGoal();
	void reachedAIGoal();
	void sustainDamage( cEntity @inflicter, cEntity @attacker, const Vec3 &in dir, float damage, float knockback, float stun, int mod );
	void splashDamage( cEntity @attacker, int radius, float damage, float knockback, float stun, int mod );
	void explosionEffect( int radius );
};

