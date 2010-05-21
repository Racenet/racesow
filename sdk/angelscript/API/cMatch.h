/**
 * cMatch
 */
class cMatch
{
public:
	/* object properties */

	/* object behaviors */

	/* object methods */
	void launchState(int state);
	void startAutorecord();
	void stopAutorecord();
	bool scoreLimitHit();
	bool timeLimitHit();
	bool isTied();
	bool checkExtendPlayTime();
	bool suddenDeathFinished();
	bool isPaused();
	bool isWaiting();
	bool isExtended();
	uint duration();
	uint startTime();
	uint endTime();
	int getState();
	cString @ getName();
	void setName( cString &in );
	void setClockOverride( uint milliseconds );
};

