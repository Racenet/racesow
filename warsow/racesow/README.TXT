/**
* Racesow README file
*
* @package Racesow
* @version 0.5.2
*/

Project goals:

    - no use of external services, instead effectively based on files
    - adapter based pattern for data-storage to allow to switch to mysql etc.
    - bring the race community together <3 (!!!!)
    - coding for fun while learning a lot
    - contribute to warsow
    
Sources:

- development trunk: http://svn.warsow-race.net/racesow0.5/trunk
- threaded mysql branch: http://svn.warsow-race.net/racesow0.5/branches/mysql_threaded
- latest stable gametypescript: http://svn.warsow-race.net/racesow0.5/tags/0.5.2-stable/warsow/racesow/progs/gametypes
- latest win32 game module: http://svn.warsow-race.net/racesow0.5/tags/0.5.2-stable/warsow/racesow/game_x86.dll
- download win32 mod package: http://downloads.warsow-race.net/warsow/racesow/racesow-0.5.2-win32.zip
    (you need to start the warsow_x86.exe or wsw_server_x86 or compile an x64 version yourself)
    
Game cvars:

    g_logRaces 1|0, default 0, log all races to a file
    g_freestyle 1|0, default 0, enables freestyle mode (not implemented yet)
    g_allowammoswitch 1|0, allows players to convert ammo
        
        
Racenet/Racesow team:

    active
        soh-zolex
        schaaf
        R2
        rlx|Schakal

    inactive
        FoV
        weqo
        valliko
        Zaran

    people who contributed before 0.5
        imhotep
        Jezus
        joko

    special thanks
        jal
        learn_more
        vic

    are you missing in this list? tell me...

    we're still searching for people interested in helping with racesow or contributing their own race modifications.
    join us in IRC #racenet@quakenet

    
    
Changelog: (items with a § require the modded game module)

    2010-05-28 / racesow 0.5.2
		- optimized overtime feature
        - diffrent scoreboards for race/freestyle
        - changed C code in order to allow noclip and position cmds
        - changed C code that projectiles can hit others in freestyle
        - added ammo to the gunblade
        - changed ammoswitch to apply on the active weapon, added g_allowammoswitch bool
        - removed drowning for freestyle
        - little improvement on angelscript commands
        - added spawnpoint randomness for freestyle avoid massacres
        - admin cancelvote function added
        - player objects get reseted on connect
        - added a floodprotection to the top command
        - added admin mute commands
        - admin remove, joinlock, votemute commands added
        - protected admins from getting callvoted
        - ugly sound when picking items fixed, changed item respawn to 0
        - fixed shootable buttons for race, added wait -1 support
        - copied trigger_delay and trigger_relay from 0.42
        - increased rockettimeout
        - support for weaponshooter entitys added
        - fixed eb to let it trigger push buttons
        - gunblade auto snipe added
        - chronometer function added
        - fixed position command to not forget your positions anymore
        - protected the name player from getting authed
        - added respawn next to the place you got telekilled feature
        - added feature for storing positions longer than one session
        - the admin command now gives you a list of players if you didn't enter enough paramters
        - now all prjectiles by a player get surpressed after respawning
        - players who are spectating you are now listed in the challengers queque
        - made the admin command shorter
        - ported the noclip command to angelscript, improved ammoswitch
        - ported the teleport command to angelscript, merged position store to it
        - added target_teleport entity support to angelscript


    2009-09-25 / racesow 0.5.1e
        - added removal of old session files
        - added creation of shadow files for registered players
        - removed goal not to use a modded game module
        - changed session to use the clients ip as filename
        - added ammoswitch       
        - protected admins from beeing kicked and callvoted etc...

    2009-09-23 / racesow 0.5.1d
        - extracted auth plugin from player model
        - added very nice nickname protection!
        - removed own md5 sources as they are in qcommon
        - removed g_secureAuth as it won't solve our problem
        - changed makefile and vcproject for md5 stuff
        - changed console command workflows

    2009-09-22 / racesow 0.5.1c
        - added fix for memory management by vic, thanks to jal §
        - added md5 encryption for passwords §
        - added g_secureAuth 0 to disable md5 encryption
        - added help command
        - changed authorization to create some subfolders
        - changed authentication to create subfolders
        - changed commands racesow_register and racesow_auth to register and auth
        - changed all files to lowercase for linux
        - fixed possible authentication exploit

    2009-09-21 / racesow 0.5.1b
        - added race-overtime feature
        - added idle detection when in overtime
        - added an abstract interface for handling race highscores
        - added basic authentication
        - added authorization
        - added admin command "map"
        - added logging of races

    2009-09-19 / racesow 0.5.1a
        - refactored whole race gameytpe (better OOP)
        - changed racerestart command so it can be used at any time
        - changed awards for checkpoints and races
        - fixed autorespawn bug when finishing a race
        
        
        
Roadmap: (items with a § require the modded game module, or wait for stuff be added to basewsw game module)

    - movement types like b1shop's mod (planned: vq3-like, cpm-like, basewsw, racesow)
    - optionally force movement type serverside
    - weapondefs, depending on movement type (see appendix for a summary of weapondefs)§
    - defrag like race-ghost for own, best and server best §
    
    
    
Appendix A: Weapondefs of warsow.5 vs racesow.42 (values for warsow.42 are not shown)

    rocket   |strong 0.5 0.42 |weak 0.5 0.42
    ---------|----------------|---------------
    damage   |       85   90  |     75   80 
    knockbk  |      100  100  |     95  100
    splash   |      140  120  |    140  120
    mindmg   |        4   15  |      4    8
    minknock |       10  none |      5  none

    (racesow.5 rocket speed and reload are already forced to racesow.42 values)

    plasma   |strong 0.5 0.42 |weak 0.5 0.42
    ---------|----------------|---------------
    damage   |       15   15  |     14   12
    selfdmg  |      0.5  0.75 |    0.5  0.5
    knockbk  |       20   28  |     14   19
    splash   |       45   40  |     45   20
    mindmg   |        5    5  |      0    1
    minknock |        1  none |      1  none
    speed    |      2.4k 1.7k |    2.4k 1.7k
    spread   |        0    0  |     90    0

    (racesow.5 weak plasma spread is already forced to 0)

    grenade  |strong 0.5 0.42 |weak 0.5 0.42
    ---------|----------------|---------------
    timeout  |     1250 2000  |   1250 2000
    damage   |       65  100  |     60  100
    selfdmg  |     0.35  0.5  |   0.35  0.5
    knockbk  |      100  120  |     90  120
    splash   |      170  150  |    160  150
    mindmg   |       15    5  |     15    5
    minknock |       10  none |      5  none
    speed    |      900  800  |    900  800

    (grenade racesow.42 reload=800, warsow.5 reload=725, should that be changed?)
