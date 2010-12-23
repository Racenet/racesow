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
	cString @ getName();
	cString @ getDefaultName();
	void setName( cString &in );
	bool isLocked();
	bool lock();
	bool unlock();
	void clearInvites();
	int team();
};

