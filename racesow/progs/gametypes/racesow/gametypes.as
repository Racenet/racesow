/**
 * Racesow_Gametype
 */
class Racesow_Gametype
{
    Racesow_Player@[] players;
    bool ammoSwitch;

    Racesow_Gametype() 
    {
        this.players = Racesow_Player@[](maxClients);
        ammoSwitch = false;
    }

    ~Racesow_Gametype() 
    {

    }

    void InitGametype() { }
    
    void SpawnGametype() { }
    
    void Shutdown() { }

    bool ammoSwitchAllowed() { return ammoSwitch; }
    
    bool MatchStateFinished( int incomingMatchState )
    {
        assert( false, "You have to overwrite 'bool MatchStateFinished( int incomingMatchState )' in your Racesow_Gametype" );
        return false;
    }
    
    void MatchStateStarted() { }
    
    void ThinkRules() { }
    
    void playerRespawn( cEntity @ent, int old_team, int new_team ) { }
    
    void scoreEvent( cClient @client, cString &score_event, cString &args ) { }
    
    cString @ScoreboardMessage( int maxlen )
    {
        assert( false, "You have to overwrite 'cString @ScoreboardMessage( int maxlen )' in your Racesow_Gametype." );
        return "This needs to be overwritten!\n";
    }
    
    cEntity @SelectSpawnPoint( cEntity @self )
    {
        assert( false, "You have to overwrite 'cEntity @SelectSpawnPoint( cEntity @self )' in your Racesow_Gametype." ); 
        return cEntity(); 
    }
    
    bool UpdateBotStatus( cEntity @self ) 
    {
        assert( false, "You have to overwrite 'bool UpdateBotStatus( cEntity @self )' in your Racesow_Gametype." ); 
        return false;
    }
    
    bool Command( cClient @client, cString @cmdString, cString @argsString, int argc )
    {
        assert( false, "You have to overwrite 'bool Command( cClient @client, cString @cmdString, cString @argsString, int argc )' in your Racesow_Gametype." ); 
        return false;
    }
}
