/* funcdefs */

/**
 * cGametypeDesc
 */
class cGametypeDesc
{
public:
	/* object properties */
	uint spawnableItemsMask;
	uint respawnableItemsMask;
	uint dropableItemsMask;
	uint pickableItemsMask;
	bool isTeamBased;
	bool isRace;
	bool hasChallengersQueue;
	int maxPlayersPerTeam;
	int ammoRespawn;
	int armorRespawn;
	int weaponRespawn;
	int healthRespawn;
	int powerupRespawn;
	int megahealthRespawn;
	int ultrahealthRespawn;
	bool readyAnnouncementEnabled;
	bool scoreAnnouncementEnabled;
	bool countdownEnabled;
	bool mathAbortDisabled;
	bool shootingDisabled;
	bool infiniteAmmo;
	bool canForceModels;
	bool canShowMinimap;
	bool teamOnlyMinimap;
	int spawnpointRadius;
	bool customDeadBodyCam;
//racesow
	bool autoInactivityRemove;
	bool playerInteraction;
	bool freestyleMapFix;
	bool enableDrowning;
//!racesow
	bool mmCompatible;

	/* object behaviors */

	/* object methods */
	String @ getName() const;
	String @ getTitle() const;
	void setTitle( String & );
	String @ getVersion() const;
	void setVersion( String & );
	String @ getAuthor() const;
	void setAuthor( String & );
	String @ getManifest() const;
	void setTeamSpawnsystem( int team, int spawnsystem, int wave_time, int wave_maxcount, bool deadcam );
	bool isInstagib() const;
	bool hasFallDamage() const;
	bool hasSelfDamage() const;
	bool isInvidualGameType() const;
};

