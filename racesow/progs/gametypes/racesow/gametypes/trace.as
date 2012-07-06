class Racesow_Gametype_Trace : Racesow_Gametype_Durace
{

    void InitGametype()
    {
        gametype.setTitle( "Team Race" );
        // if the gametype doesn't have a config file, create it
        if ( !G_FileExists( "configs/server/gametypes/trace.cfg" ) )
        {
            String config;

            // the config file doesn't exist or it's empty, create it
            config = "// '" + gametype.getTitle() + "' gametype configuration file\n"
                     + "// This config will be executed each time the gametype is started\n"
                     + "\n// game settings\n"
                     + "set g_scorelimit \"0\"\n"
                     + "set g_timelimit \"10\"\n"
                     + "set g_warmup_enabled \"1\"\n"
                     + "set g_warmup_timelimit \"3\"\n"
                     + "set g_match_extendedtime \"0\"\n"
                     + "set g_teams_maxplayers \"8\"\n"
                     + "set g_teams_allow_uneven \"0\"\n"
                     + "set g_countdown_time \"3\"\n"
                     + "set g_maxtimeouts \"-1\" // -1 = unlimited\n"
                     + "set g_challengers_queue \"0\"\n"
                     
                     + "\necho trace.cfg executed\"\n";

            G_WriteFile( "configs/server/gametypes/trace.cfg", config );
            G_Print( "Created default config file for 'trace'\n" );
            G_CmdExecute( "exec configs/server/gametypes/trace.cfg silent" );
        }

        gametype.hasChallengersQueue = false;
        
        gametype.isTeamBased = true;
        gametype.spawnpointRadius = 0;
        gametype.mathAbortDisabled = false;
        gametype.autoInactivityRemove = true;
        gametype.playerInteraction = false;
        gametype.freestyleMapFix = false;
        gametype.enableDrowning = true;

        //store the timelimit because value in DURACE is not the same than in RACE
        oldTimelimit = g_timelimit.getInteger();
            
        // define the scoreboard layout
        G_ConfigString( CS_SCB_PLAYERTAB_LAYOUT, "%n 112 %s 52 %i 52 %t 96 %l 48 %b 50 %p 18" );
        G_ConfigString( CS_SCB_PLAYERTAB_TITLES, "Name Clan Score Time Ping Racing R" );
    
        G_Print( "Gametype '" + gametype.getTitle() + "' initialized\n" );
    }
}

Racesow_Gametype@ getRacesowGametype() {
    return @Racesow_Gametype_Trace();
}
