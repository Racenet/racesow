/**
 * Racesow_Gametype
 */
class Racesow_Gametype
{
    Racesow_Player@[] players;

    Racesow_Gametype() 
    {
        this.players = Racesow_Player@[](maxClients);
    }

    ~Racesow_Gametype() 
    {

    }

    void InitGametype() { }
    
    void SpawnGametype() { }
    
    void Shutdown() { }
    
    bool MatchStateFinished( int incomingMatchState ) { return false; }
    
    void MatchStateStarted() { }
    
    void ThinkRules() { }
    
    void playerRespawn( cEntity @ent, int old_team, int new_team ) { }
    
    void scoreEvent( cClient @client, cString &score_event, cString &args ) { }
    
    cString @ScoreboardMessage( int maxlen ) { return "This needs to be overwritten!\n"; }
    
    cEntity @SelectSpawnPoint( cEntity @self ) { return cEntity(); }
    
    bool UpdateBotStatus( cEntity @self ) { return false; }
    
    bool Command( cClient @client, cString @cmdString, cString @argsString, int argc ) { return false; }
}
