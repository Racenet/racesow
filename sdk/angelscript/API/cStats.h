/**
 * cStats
 */
class cStats
{
public:
	/* object properties */
	const int score;
	const int deaths;
	const int frags;
	const int suicides;
	const int teamFrags;
	const int awards;
	const int totalDamageGiven;
	const int totalDamageReceived;
	const int totalTeamDamageGiven;
	const int totalTeamDamageReceived;
	const int healthTaken;
	const int armorTaken;

	/* object behaviors */
	cStats @ f(); /* factory */ 

	/* object methods */
	void setScore( int i );
	void addScore( int i );
	void clear();
	int accuracyShots( int ammo );
	int accuracyHits( int ammo );
	int accuracyHitsDirect( int ammo );
	int accuracyHitsAir( int ammo );
	int accuracyDamage( int ammo );
};

