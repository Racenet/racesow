/* funcdefs */

/**
 * cTeam
 */
class cTeam
{
public:
	/* object properties */
	cStats stats;
	const int numPlayers;
	const int ping;
	const bool hasCoach;

	/* object behaviors */
	cTeam @ f(); /* factory */ 

	/* object methods */
	cEntity @ ent( int index );
	String @ getName() const;
	String @ getDefaultName() const;
	void setName( String &in );
	bool isLocked() const;
	bool lock() const;
	bool unlock() const;
	void clearInvites();
	int team() const;
};

