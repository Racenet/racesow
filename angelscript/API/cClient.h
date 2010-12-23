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
	const int muted;
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

	/* object behaviors */
	cClient @ f(); /* factory */ 

	/* object methods */
	int playerNum();
	bool isReady();
	bool isBot();
	cBot @ getBot();
	int state();
	void respawn( bool ghost );
	void clearPlayerStateEvents();
	cString @ getName();
	cString @ getClanName();
	cEntity @ getEnt();
	int inventoryCount( int tag );
	void inventorySetCount( int tag, int count );
	void inventoryGiveItem( int tag, int count );
	void inventoryGiveItem( int tag );
	void inventoryClear();
	bool canSelectWeapon( int tag );
	void selectWeapon( int tag );
	void addAward( cString &in );
	void execGameCommand( cString &in );
	void setHUDStat( int stat, int value );
	int getHUDStat( int stat );
	void setPMoveFeatures( uint bitmask );
	void setPMoveMaxSpeed( float speed );
	void setPMoveJumpSpeed( float speed );
	void setPMoveDashSpeed( float speed );
	cString @ getUserInfoKey( cString &in );
	void printMessage( cString &in );
	void chaseCam( cString @, bool teamOnly );
};

