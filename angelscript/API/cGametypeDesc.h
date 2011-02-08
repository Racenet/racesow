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
	// racesow
    bool autoInactivityRemove;
    bool playerInteraction;
    bool freestyleMapFix;
    bool enableDrowning;
	// ! racesow

    /* object behaviors */

    /* object methods */
    cString @ getName();
    cString @ getTitle();
    void setTitle( cString & );
    cString @ getVersion();
    void setVersion( cString & );
    cString @ getAuthor();
    void setAuthor( cString & );
    cString @ getManifest();
    void setTeamSpawnsystem( int team, int spawnsystem, int wave_time, int wave_maxcount, bool deadcam );
    bool isInstagib();
    bool hasFallDamage();
    bool hasSelfDamage();
    bool isInvidualGameType();
};
