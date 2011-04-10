/**
 * Racesow_Gametype interface
 */
interface Racesow_Gametype
{
    void InitGametype();
    
    void SpawnGametype();
    
    void Shutdown();
    
    bool MatchStateFinished( int incomingMatchState );
    
    void MatchStateStarted();
    
    void ThinkRules();
    
    void playerRespawn( cEntity @ent, int old_team, int new_team );
    
    void scoreEvent( cClient @client, cString &score_event, cString &args );
    
    cString @ScoreboardMessage( int maxlen );
    
    cEntity @SelectSpawnPoint( cEntity @self );
    
    bool UpdateBotStatus( cEntity @self );
    
    bool Command( cClient @client, cString @cmdString, cString @argsString, int argc );
}
