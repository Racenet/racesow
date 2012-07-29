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
    
    void scoreEvent( cClient @client, String &score_event, String &args );
    
    String @ScoreboardMessage( uint maxlen );
    
    cEntity @SelectSpawnPoint( cEntity @self );
    
    bool UpdateBotStatus( cEntity @self );
    
    bool Command( cClient @client, String @cmdString, String @argsString, int argc );
}
