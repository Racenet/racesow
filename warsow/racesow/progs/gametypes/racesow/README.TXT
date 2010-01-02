/**
 * Racesow README file
 *
 * @package Racesow
 * @version 0.5.1c
 * @date 24.09.2009
 * @author soh-zolex <zolex@warsow-race.net>
 */

    Project goals:

        - no use of external services, instead effectively (!) based on files
        - bring the race community together <3 (!!!!)
        - coding for fun while learning a lot
        - contribute to warsow

		
		
    Sources:

	- development trunk: http://svn.warsow-race.net/racesow0.5/trunk/[/url]
	- latest stable gametypescript: http://svn.warsow-race.net/racesow0.5/tags/0.5.1d-stable/warsow/racesow/progs/gametypes/
	- latest win32 game module: http://svn.warsow-race.net/racesow0.5/trunk/sdk/source/Release/x86/basewsw/game_x86.dll
		
    Game cvars:

        g_logRaces 1|0, default 0, log all races to a file
        g_freestyle 1|0, default 0, enables freestyle mode (not implemented yet)

		
		
    Game commands:
       
        racerestart, go back to the start-area whenever you want
        register <name> <email> <password> <password confirmation>
        auth <name> <password>
        admin <command params>
            admin map <map name>
	    admin kick <playerNum>
	    admin help
        help, shows the help

        you can add this to your autoexec.cfg to auto-login when connecting to a racesow server
            setu auth_login "name"
            setu auth_pass "password"

			
			
    Racenet/Racesow team:

        core team
            FoV: development (racesow), takes gameserver costs <3
            soh-zolex: project founder, management, public relations, lead coder, server costs

        inactive:
            weqo: development (racenet)

        ppl currently getting into
            R2: development (racesow)
            valliko: development (racesow)
            Zaran: development (racenet)

        ppl who contributed before 0.5
            imhotep
            Jezus
            joko

	special thanks to
            jal
            learn_more
            vic

        are you missing in this list? tell me...

        we're still searching for people interested in helping with racesow or contributing their own race modifications.
        join us in IRC #racenet@quakenet

		
		
    Changelog: (items with a � require the modded game module)
	
        2009-09-25 / racesow 0.5.1d
            - added removal of old session files
            - added creation of shadow files for registered players
            - removed goal not to use a modded game module
            - changed session to use the clients ip as filename

        2009-09-23 / racesow 0.5.1c
            - extracted auth plugin from player model
            - added very nice nickname protection!
            - removed own md5 sources as they are in qcommon
    	    - removed g_secureAuth as it won't solve our problem
            - changed makefile and vcproject for md5 stuff
            - changed console command workflows

        2009-09-22 / racesow 0.5.1b
            - added fix for memory management by vic, thanks to jal �
            - added md5 encryption for passwords �
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
			
			
			
	Roadmap: (items with a � require the modded game module, or wait for stuff be added to basewsw game module)
   
        - movement types like b1shop's mod (planned: vq3-like, cpm-like, basewsw, racesow)
        - optionally force movement type serverside
        - weapondefs, depending on movement type (see appendix for a summary of weapondefs)�
        - let player choose to shoot weak or strong ammo
        - defrag like race-ghost for own, best and server best �
        - protect admins from beeing kicked and callvoted etc...
		
		
	Appendix: weapondefs of warsow.5 vs racesow.42 (values for warsow.42 are not shown)

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

	