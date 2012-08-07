/* funcdefs */

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
	void addRound();
	void clear();
	int accuracyShots( int ammo ) const;
	int accuracyHits( int ammo ) const;
	int accuracyHitsDirect( int ammo ) const;
	int accuracyHitsAir( int ammo ) const;
	int accuracyDamage( int ammo ) const;
};

