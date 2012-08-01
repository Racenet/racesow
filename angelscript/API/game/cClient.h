/* funcdefs */

/**
 * cClient
 */
class cClient
{
public:
	/* object properties */
	cStats stats;
	const bool connecting;
	const bool multiview;
	const bool tv;
	int team;
	const int hand;
	const int fov;
	const int zoomFov;
	const bool isOperator;
	const uint queueTimeStamp;
//racesow
//	const int muted;
	int muted;
//!racesow
	float armor;
	uint gunbladeChargeTimeStamp;
	const bool chaseActive;
	int chaseTarget;
	bool chaseTeamonly;
	int chaseFollowMode;
	const bool coach;
	const int ping;
	const int16 weapon;
	const int16 pendingWeapon;
	const int16 pmoveFeatures;
	bool takeStun;
	uint lastActivity;

	/* object behaviors */
	cClient @ f(); /* factory */ 

	/* object methods */
	int playerNum() const;
	bool isReady() const;
	bool isBot() const;
	cBot @ getBot() const;
	int state() const;
	void respawn( bool ghost );
	void clearPlayerStateEvents();
	String @ getName() const;
	String @ getClanName() const;
	cEntity @ getEnt() const;
	int inventoryCount( int tag ) const;
	void inventorySetCount( int tag, int count );
	void inventoryGiveItem( int tag, int count );
	void inventoryGiveItem( int tag );
	void inventoryClear();
	bool canSelectWeapon( int tag ) const;
	void selectWeapon( int tag );
	void addAward( const String &in );
	void addMetaAward( const String &in );
	void execGameCommand( const String &in );
	void setHUDStat( int stat, int value );
	int getHUDStat( int stat ) const;
	void setPMoveFeatures( uint bitmask );
	void setPMoveMaxSpeed( float speed );
	void setPMoveJumpSpeed( float speed );
	void setPMoveDashSpeed( float speed );
	String @ getUserInfoKey( const String &in ) const;
	void printMessage( const String &in );
	void chaseCam( String @, bool teamOnly );
	void setChaseActive( bool active );
	void newRaceRun( int numSectors );
	void setRaceTime( int sector, uint time );
};

