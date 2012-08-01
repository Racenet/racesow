/* funcdefs */

/**
 * cMatch
 */
class cMatch
{
public:
	/* object properties */

	/* object behaviors */

	/* object methods */
	void launchState(int state) const;
	void startAutorecord() const;
	void stopAutorecord() const;
	bool scoreLimitHit() const;
	bool timeLimitHit() const;
	bool isTied() const;
	bool checkExtendPlayTime() const;
	bool suddenDeathFinished() const;
	bool isPaused() const;
	bool isWaiting() const;
	bool isExtended() const;
	uint duration() const;
	uint startTime() const;
	uint endTime() const;
	int getState() const;
	String @ getName() const;
	String @ getScore() const;
	void setName( String &in );
	void setScore( String &in );
	void setClockOverride( uint milliseconds );
};

